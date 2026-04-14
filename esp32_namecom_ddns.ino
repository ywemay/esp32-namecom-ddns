/*
  ESP32-C3 Name.com Dynamic DNS Updater with Secure Configuration
  Updates an A record on Name.com with the current external IP address
  
  Features:
  - NVS-based secure credential storage (no hardcoded credentials)
  - Access Point mode for initial setup (no password required)
  - LAN mode with password protection for configuration changes
  - Captive portal for easy setup
  - Hardware trigger (GPIO2 HIGH) for emergency config access
  - Factory reset capability
  
  Requirements:
  - WiFi connection
  - Name.com API credentials (stored securely in NVS)
  - Domain registered with Name.com
  
  API Documentation: https://www.name.com/api-docs
*/

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// Include configuration
#include "config.h"

// Global objects
Preferences preferences;
WebServer server(WEB_SERVER_PORT);
DNSServer dnsServer;

// Configuration stored in NVS
String wifiSSID = "";
String wifiPassword = "";
String nameComUsername = "";
String nameComApiToken = "";
String domainName = "";
String recordHost = "@";
String recordID = "";
String configPassword = "";  // Password for LAN-based config access
unsigned int updateIntervalMinutes = DEFAULT_UPDATE_INTERVAL_MINUTES;
unsigned int retryIntervalSeconds = DEFAULT_RETRY_INTERVAL_SECONDS;
unsigned int ttlSeconds = DEFAULT_TTL_SECONDS;

// Runtime state
String currentExternalIP = "";
unsigned long lastUpdateTime = 0;
unsigned long lastIPCheckTime = 0;
bool wifiConnected = false;
bool apMode = false;
bool configChanged = false;

// Forward declarations
void saveConfigToNVS();
void loadConfigFromNVS();
void resetConfigToDefaults();
bool validateConfigPassword(const String& password);
void startAPMode();
void startWebServer();
void stopAPMode();
void handleRoot();
void handleConfig();
void handleStatus();
void handleReset();
void handleSave();
void sendConfigPage();
void sendStatusPage();
void sendResetPage();
String base64Encode(String data);
String getBasicAuthHeader();
void connectToWiFi();
String getExternalIP();
bool updateDNSRecord(String ipAddress);
void checkAndConnect();

/*
 * Base64 encoding function for Basic Auth
 */
String base64Encode(String data) {
  const char* input = data.c_str();
  String encoded = "";
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];
  const char* encoding_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  
  while (strlen(input)) {
    char_array_3[i++] = *(input++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;
      
      for(i = 0; i < 4; i++) {
        encoded += encoding_table[char_array_4[i]];
      }
      i = 0;
    }
  }
  
  if (i) {
    for(j = i; j < 3; j++) {
      char_array_3[j] = '\0';
    }
    
    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;
    
    for(j = 0; j < i + 1; j++) {
      encoded += encoding_table[char_array_4[j]];
    }
    
    while(i++ < 3) {
      encoded += '=';
    }
  }
  
  return encoded;
}

/*
 * Get Basic Auth header value using stored credentials
 */
String getBasicAuthHeader() {
  if (nameComUsername.length() == 0 || nameComApiToken.length() == 0) {
    return "";
  }
  String credentials = nameComUsername + ":" + nameComApiToken;
  return "Basic " + base64Encode(credentials);
}

/*
 * Save all configuration to NVS
 */
void saveConfigToNVS() {
  preferences.begin(NVS_NAMESPACE, false);
  
  preferences.putString("wifiSSID", wifiSSID);
  preferences.putString("wifiPassword", wifiPassword);
  preferences.putString("nameComUsername", nameComUsername);
  preferences.putString("nameComApiToken", nameComApiToken);
  preferences.putString("domainName", domainName);
  preferences.putString("recordHost", recordHost);
  preferences.putString("recordID", recordID);
  preferences.putString("configPassword", configPassword);
  preferences.putUInt("updateInterval", updateIntervalMinutes);
  preferences.putUInt("retryInterval", retryIntervalSeconds);
  preferences.putUInt("ttl", ttlSeconds);
  
  preferences.end();
  
  #ifdef DEBUG_MODE
  Serial.println("Configuration saved to NVS");
  #endif
}

/*
 * Load all configuration from NVS
 */
void loadConfigFromNVS() {
  preferences.begin(NVS_NAMESPACE, false);
  
  wifiSSID = preferences.getString("wifiSSID", "");
  wifiPassword = preferences.getString("wifiPassword", "");
  nameComUsername = preferences.getString("nameComUsername", "");
  nameComApiToken = preferences.getString("nameComApiToken", "");
  domainName = preferences.getString("domainName", "");
  recordHost = preferences.getString("recordHost", "@");
  recordID = preferences.getString("recordID", "");
  configPassword = preferences.getString("configPassword", "");
  updateIntervalMinutes = preferences.getUInt("updateInterval", DEFAULT_UPDATE_INTERVAL_MINUTES);
  retryIntervalSeconds = preferences.getUInt("retryInterval", DEFAULT_RETRY_INTERVAL_SECONDS);
  ttlSeconds = preferences.getUInt("ttl", DEFAULT_TTL_SECONDS);
  
  preferences.end();
  
  #ifdef DEBUG_MODE
  Serial.println("Configuration loaded from NVS");
  Serial.print("WiFi SSID: ");
  Serial.println(wifiSSID.length() > 0 ? wifiSSID : "(not configured)");
  Serial.print("Domain: ");
  Serial.println(domainName.length() > 0 ? domainName : "(not configured)");
  #endif
}

/*
 * Reset all configuration to defaults and clear NVS
 */
void resetConfigToDefaults() {
  preferences.begin(NVS_NAMESPACE, false);
  preferences.clear();
  preferences.end();
  
  // Reload defaults
  loadConfigFromNVS();
  
  #ifdef DEBUG_MODE
  Serial.println("Configuration reset to defaults");
  #endif
}

/*
 * Validate configuration password
 */
bool validateConfigPassword(const String& password) {
  if (configPassword.length() == 0) {
    return true;  // No password set
  }
  return password == configPassword;
}

/*
 * Start Access Point mode with captive portal
 */
void startAPMode() {
  apMode = true;
  
  // Generate unique AP SSID with chip ID
  String apSSID = String(AP_SSID_PREFIX) + String((uint32_t)ESP.getEfuseMac(), HEX);
  apSSID.toUpperCase();
  
  #ifdef DEBUG_MODE
  Serial.print("Starting AP Mode with SSID: ");
  Serial.println(apSSID);
  #endif
  
  WiFi.softAP(apSSID.c_str(), NULL, AP_CHANNEL, AP_MAX_CONNECTIONS);
  
  // Start DNS server for captive portal
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
  
  // Start web server
  startWebServer();
  
  #ifdef DEBUG_MODE
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Captive portal started");
  #endif
}

/*
 * Stop Access Point mode
 */
void stopAPMode() {
  if (apMode) {
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    apMode = false;
    
    #ifdef DEBUG_MODE
    Serial.println("AP Mode stopped");
    #endif
  }
}

/*
 * Start web server with all routes
 */
void startWebServer() {
  // Root - redirect to config or status
  server.on("/", []() {
    if (apMode || !wifiConnected) {
      sendConfigPage();
    } else {
      sendStatusPage();
    }
  });
  
  // Configuration page
  server.on("/config", []() {
    // In AP mode, no password required
    // In LAN mode, check password
    String pwd = server.arg("password");
    if (!apMode && !validateConfigPassword(pwd)) {
      server.send(200, "text/html", "<html><head><meta http-equiv=\"refresh\" content=\"0;url=/\"></head><body>Password required. <a href=\"/\">Return</a></body></html>");
      return;
    }
    sendConfigPage();
  });
  
  // Status page
  server.on("/status", []() {
    sendStatusPage();
  });
  
  // Reset page
  server.on("/reset", []() {
    if (!apMode) {
      String pwd = server.arg("password");
      if (!validateConfigPassword(pwd)) {
        server.send(200, "text/html", "<html><head><meta http-equiv=\"refresh\" content=\"0;url=/\"></head><body>Password required. <a href=\"/\">Return</a></body></html>");
        return;
      }
    }
    sendResetPage();
  });
  
  // Save configuration
  server.on("/save", HTTP_POST, []() {
    if (!apMode && server.hasArg("password")) {
      if (!validateConfigPassword(server.arg("password"))) {
        server.send(403, "text/plain", "Invalid password");
        return;
      }
    }
    handleSave();
  });
  
  // Factory reset
  server.on("/factory-reset", HTTP_POST, []() {
    if (!apMode) {
      String pwd = server.arg("password");
      if (!validateConfigPassword(pwd)) {
        server.send(403, "text/plain", "Invalid password");
        return;
      }
    }
    resetConfigToDefaults();
    server.send(200, "text/html", "<html><head><meta http-equiv=\"refresh\" content=\"5;url=/config\"></head><body><h1>Factory Reset Complete</h1><p>Device will restart in AP mode. Reconnect to ESP32-DDNS-XXXX network.</p><p>Restarting...</p></body></html>");
    delay(2000);
    ESP.restart();
  });
  
  // Reboot
  server.on("/reboot", HTTP_POST, []() {
    server.send(200, "text/html", "<html><head><meta http-equiv=\"refresh\" content=\"5;url=/\"></head><body><h1>Rebooting...</h1><p>Please wait...</p></body></html>");
    delay(2000);
    ESP.restart();
  });
  
  server.begin();
  
  #ifdef DEBUG_MODE
  Serial.println("Web server started");
  #endif
}

/*
 * Send configuration page HTML
 */
void sendConfigPage() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ESP32 DDNS Config</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>body{font-family:Arial,sans-serif;max-width:600px;margin:20px auto;padding:20px;}";
  html += "h1{color:#333;} input,select{width:100%;padding:8px;margin:5px 0 15px 0;border:1px solid #ddd;border-radius:4px;box-sizing:border-box;}";
  html += "button{background:#4CAF50;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;width:100%;font-size:16px;}";
  html += "button:hover{background:#45a049;} .section{margin-bottom:20px;padding:15px;border:1px solid #ddd;border-radius:4px;}";
  html += ".section h2{margin-top:0;color:#555;} .info{background:#e7f3fe;border-left:4px solid #2196F3;padding:10px;margin-bottom:15px;}";
  html += ".warning{background:#fff3cd;border-left:4px solid #ffc107;padding:10px;margin-bottom:15px;}</style></head><body>";
  
  html += "<h1>⚡ ESP32 DDNS Configuration</h1>";
  
  if (apMode) {
    html += "<div class='info'><strong>Setup Mode:</strong> Configure your device. No password required for initial setup.</div>";
  } else {
    html += "<div class='info'><strong>LAN Mode:</strong> Enter password to modify settings.</div>";
    html += "<form method='GET' action='/config'><input type='password' name='password' placeholder='Configuration Password' required><button type='submit'>Unlock</button></form><hr>";
  }
  
  html += "<form method='POST' action='/save'>";
  
  if (!apMode) {
    html += "<input type='hidden' name='password' value='" + server.arg("password") + "'>";
  }
  
  // WiFi Section
  html += "<div class='section'><h2>📶 WiFi Configuration</h2>";
  html += "<label>WiFi SSID:</label><input type='text' name='wifiSSID' value='" + wifiSSID + "' required>";
  html += "<label>WiFi Password:</label><input type='password' name='wifiPassword' value='" + wifiPassword + "'>";
  html += "</div>";
  
  // Name.com Section
  html += "<div class='section'><h2>🌐 Name.com API</h2>";
  html += "<div class='warning'>Get your API token from: <a href='https://www.name.com/account/settings/api' target='_blank'>Name.com Account Settings</a></div>";
  html += "<label>Username:</label><input type='text' name='nameComUsername' value='" + nameComUsername + "' required>";
  html += "<label>API Token:</label><input type='password' name='nameComApiToken' value='" + nameComApiToken + "' required>";
  html += "</div>";
  
  // Domain Section
  html += "<div class='section'><h2>🏷️ Domain Configuration</h2>";
  html += "<label>Domain Name:</label><input type='text' name='domainName' value='" + domainName + "' placeholder='example.com' required>";
  html += "<label>Record Host:</label><input type='text' name='recordHost' value='" + recordHost + "' placeholder='@ for root, or subdomain'>";
  html += "<label>Record ID:</label><input type='text' name='recordID' value='" + recordID + "' required>";
  html += "</div>";
  
  // Security Section
  html += "<div class='section'><h2>🔐 Security</h2>";
  html += "<label>Config Password:</label><input type='password' name='configPassword' value='" + configPassword + "' placeholder='Leave empty for no password'>";
  html += "<small>Password required for LAN-based configuration changes</small>";
  html += "</div>";
  
  // Advanced Section
  html += "<div class='section'><h2>⚙️ Advanced</h2>";
  html += "<label>Update Interval (minutes):</label><input type='number' name='updateInterval' value='" + String(updateIntervalMinutes) + "' min='1' max='60'>";
  html += "<label>Retry Interval (seconds):</label><input type='number' name='retryInterval' value='" + String(retryIntervalSeconds) + "' min='10' max='300'>";
  html += "<label>TTL (seconds):</label><input type='number' name='ttl' value='" + String(ttlSeconds) + "' min='60' max='3600'>";
  html += "</div>";
  
  html += "<button type='submit'>💾 Save Configuration</button>";
  html += "</form>";
  
  html += "<hr><h3>📊 Device Status</h3>";
  html += "<p><strong>IP Address:</strong> " + (wifiConnected ? WiFi.localIP().toString() : "Not connected") + "</p>";
  html += "<p><strong>Mode:</strong> " + String(apMode ? "Access Point" : "Station") + "</p>";
  html += "<p><strong>MAC:</strong> " + String((uint32_t)ESP.getEfuseMac(), HEX) + "</p>";
  
  html += "<hr><form method='GET' action='/reset' style='display:inline;'><button type='submit' style='background:#f44336;'>🔄 Factory Reset</button></form>";
  html += " <form method='POST' action='/reboot' style='display:inline;'><button type='submit' style='background:#2196F3;'>🔌 Reboot</button></form>";
  
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

/*
 * Send status page HTML
 */
void sendStatusPage() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ESP32 DDNS Status</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<meta http-equiv='refresh' content='30'>";  // Auto-refresh every 30 seconds
  html += "<style>body{font-family:Arial,sans-serif;max-width:600px;margin:20px auto;padding:20px;}";
  html += "h1{color:#333;} .status{padding:15px;margin:10px 0;border-radius:4px;}";
  html += ".ok{background:#d4edda;border-left:4px solid #28a745;} .warn{background:#fff3cd;border-left:4px solid #ffc107;}";
  html += ".err{background:#f8d7da;border-left:4px solid #dc3545;} .info{background:#e7f3fe;border-left:4px solid #2196F3;}";
  html += "table{width:100%;border-collapse:collapse;margin:15px 0;} th,td{padding:10px;text-align:left;border-bottom:1px solid #ddd;}";
  html += "th{background:#f5f5f5;} a{color:#2196F3;} button{background:#4CAF50;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;margin:5px;}";
  html += "button:hover{background:#45a049;}</style></head><body>";
  
  html += "<h1>⚡ ESP32 DDNS Status</h1>";
  
  // Connection status
  if (wifiConnected) {
    html += "<div class='status ok'><strong>✓ WiFi Connected</strong></div>";
  } else {
    html += "<div class='status err'><strong>✗ WiFi Disconnected</strong></div>";
  }
  
  // DDNS status
  if (currentExternalIP.length() > 0) {
    html += "<div class='status ok'><strong>✓ DDNS Active</strong><br>Current IP: " + currentExternalIP + "</div>";
  } else {
    html += "<div class='status warn'><strong>⚠ DDNS Not Updated</strong><br>No external IP detected yet</div>";
  }
  
  // Configuration summary
  html += "<h2>📋 Configuration</h2>";
  html += "<table>";
  html += "<tr><th>Setting</th><th>Value</th></tr>";
  html += "<tr><td>WiFi SSID</td><td>" + (wifiSSID.length() > 0 ? wifiSSID : "Not configured") + "</td></tr>";
  html += "<tr><td>Domain</td><td>" + (domainName.length() > 0 ? domainName : "Not configured") + "</td></tr>";
  html += "<tr><td>Record Host</td><td>" + recordHost + "</td></tr>";
  html += "<tr><td>Record ID</td><td>" + (recordID.length() > 0 ? recordID : "Not configured") + "</td></tr>";
  html += "<tr><td>Update Interval</td><td>" + String(updateIntervalMinutes) + " minutes</td></tr>";
  html += "<tr><td>Last Update</td><td>" + (lastUpdateTime > 0 ? String((millis() - lastUpdateTime) / 1000) + "s ago" : "Never") + "</td></tr>";
  html += "</table>";
  
  // Network info
  html += "<h2>🌐 Network Information</h2>";
  html += "<table>";
  html += "<tr><th>Property</th><th>Value</th></tr>";
  html += "<tr><td>Local IP</td><td>" + (wifiConnected ? WiFi.localIP().toString() : "N/A") + "</td></tr>";
  html += "<tr><td>Subnet Mask</td><td>" + (wifiConnected ? WiFi.subnetMask().toString() : "N/A") + "</td></tr>";
  html += "<tr><td>Gateway</td><td>" + (wifiConnected ? WiFi.gatewayIP().toString() : "N/A") + "</td></tr>";
  html += "<tr><td>DNS Server</td><td>" + (wifiConnected ? WiFi.dnsIP().toString() : "N/A") + "</td></tr>";
  html += "<tr><td>MAC Address</td><td>" + String((uint32_t)ESP.getEfuseMac(), HEX) + "</td></tr>";
  html += "<tr><td>RSSI</td><td>" + (wifiConnected ? String(WiFi.RSSI()) + " dBm" : "N/A") + "</td></tr>";
  html += "</table>";
  
  // Actions
  html += "<h2>⚙️ Actions</h2>";
  html += "<form method='GET' action='/config'><button type='submit'>📝 Configure</button></form>";
  html += "<form method='GET' action='/reset'><button type='submit'>🔄 Factory Reset</button></form>";
  html += "<form method='POST' action='/reboot'><button type='submit'>🔌 Reboot</button></form>";
  
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

/*
 * Send reset confirmation page
 */
void sendResetPage() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Factory Reset</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>body{font-family:Arial,sans-serif;max-width:600px;margin:20px auto;padding:20px;}";
  html += "h1{color:#dc3545;} .warning{background:#fff3cd;border:2px solid #ffc107;padding:20px;margin:20px 0;border-radius:4px;}";
  html += "button{padding:15px 30px;margin:10px;border:none;border-radius:4px;cursor:pointer;font-size:16px;}";
  html += ".confirm{background:#dc3545;color:white;} .cancel{background:#6c757d;color:white;}</style></head><body>";
  
  html += "<h1>⚠️ Factory Reset</h1>";
  
  html += "<div class='warning'>";
  html += "<strong>WARNING:</strong> This will erase all configuration including:<br><br>";
  html += "• WiFi credentials<br>";
  html += "• Name.com API credentials<br>";
  html += "• Domain configuration<br>";
  html += "• All stored settings<br><br>";
  html += "The device will restart in Access Point mode after reset.";
  html += "</div>";
  
  html += "<form method='POST' action='/factory-reset'>";
  if (!apMode) {
    html += "<input type='hidden' name='password' value='" + server.arg("password") + "'>";
  }
  html += "<button type='submit' class='confirm'>⚠️ Yes, Factory Reset</button>";
  html += "</form>";
  
  html += "<form method='GET' action='/'>";
  html += "<button type='submit' class='cancel'>Cancel</button>";
  html += "</form>";
  
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

/*
 * Handle save configuration form
 */
void handleSave() {
  #ifdef DEBUG_MODE
  Serial.println("Saving configuration...");
  #endif
  
  // WiFi
  if (server.hasArg("wifiSSID")) {
    wifiSSID = server.arg("wifiSSID");
  }
  if (server.hasArg("wifiPassword")) {
    wifiPassword = server.arg("wifiPassword");
  }
  
  // Name.com
  if (server.hasArg("nameComUsername")) {
    nameComUsername = server.arg("nameComUsername");
  }
  if (server.hasArg("nameComApiToken")) {
    nameComApiToken = server.arg("nameComApiToken");
  }
  
  // Domain
  if (server.hasArg("domainName")) {
    domainName = server.arg("domainName");
  }
  if (server.hasArg("recordHost")) {
    recordHost = server.arg("recordHost");
  }
  if (server.hasArg("recordID")) {
    recordID = server.arg("recordID");
  }
  
  // Security
  if (server.hasArg("configPassword")) {
    configPassword = server.arg("configPassword");
  }
  
  // Advanced
  if (server.hasArg("updateInterval")) {
    updateIntervalMinutes = server.arg("updateInterval").toInt();
  }
  if (server.hasArg("retryInterval")) {
    retryIntervalSeconds = server.arg("retryInterval").toInt();
  }
  if (server.hasArg("ttl")) {
    ttlSeconds = server.arg("ttl").toInt();
  }
  
  // Save to NVS
  saveConfigToNVS();
  
  // Try to connect to WiFi if credentials changed
  if (wifiSSID.length() > 0 && !apMode) {
    #ifdef DEBUG_MODE
    Serial.println("Attempting to connect to WiFi with new credentials...");
    #endif
    WiFi.disconnect();
    delay(500);
    connectToWiFi();
  }
  
  // Send success response
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Configuration Saved</title>";
  html += "<meta http-equiv='refresh' content='3;url=/status'>";
  html += "<style>body{font-family:Arial,sans-serif;max-width:600px;margin:20px auto;padding:20px;text-align:center;}";
  html += "h1{color:#28a745;} .success{background:#d4edda;border:2px solid #28a745;padding:20px;margin:20px 0;border-radius:4px;}";
  html += "a{color:#2196F3;}</style></head><body>";
  
  html += "<h1>✓ Configuration Saved</h1>";
  html += "<div class='success'>Your settings have been saved successfully.</div>";
  html += "<p>Redirecting to status page...</p>";
  html += "<p>If not redirected, <a href='/status'>click here</a>.</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
  
  configChanged = true;
}

/*
 * Connect to WiFi using stored credentials
 */
void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    return;
  }
  
  if (wifiSSID.length() == 0) {
    #ifdef DEBUG_MODE
    Serial.println("No WiFi credentials configured");
    #endif
    wifiConnected = false;
    return;
  }
  
  #ifdef DEBUG_MODE
  Serial.print("Connecting to WiFi: ");
  Serial.println(wifiSSID);
  #endif
  
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    #ifdef DEBUG_MODE
    Serial.print(".");
    #endif
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    #ifdef DEBUG_MODE
    Serial.println("\nWiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    #endif
  } else {
    wifiConnected = false;
    #ifdef DEBUG_MODE
    Serial.println("\nFailed to connect to WiFi");
    #endif
  }
}

/*
 * Check and connect to WiFi or start AP mode
 */
void checkAndConnect() {
  // Check if GPIO trigger is active (GPIO2 pulled HIGH)
  bool gpioTrigger = (digitalRead(CONFIG_BUTTON_PIN) == HIGH);
  
  // Check if WiFi credentials exist
  bool hasCredentials = (wifiSSID.length() > 0);
  
  if (!hasCredentials || gpioTrigger) {
    #ifdef DEBUG_MODE
    Serial.print("Starting AP Mode - ");
    Serial.println(!hasCredentials ? "No credentials" : "GPIO trigger");
    #endif
    startAPMode();
    return;
  }
  
  // Try to connect to WiFi
  connectToWiFi();
  
  if (!wifiConnected) {
    #ifdef DEBUG_MODE
    Serial.println("WiFi connection failed, starting AP Mode");
    #endif
    startAPMode();
  }
}

/*
 * Get external IP address from a service
 */
String getExternalIP() {
  HTTPClient http;
  String ip = "";
  
  const char* services[] = {
    IP_CHECK_SERVICE_1,
    IP_CHECK_SERVICE_2,
    IP_CHECK_SERVICE_3
  };
  
  for (int i = 0; i < 3; i++) {
    #ifdef DEBUG_MODE
    Serial.print("Checking IP from service ");
    Serial.println(i + 1);
    #endif
    
    http.begin(services[i]);
    http.setTimeout(5000);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
      ip = http.getString();
      ip.trim();
      
      if (ip.length() > 6 && ip.length() < 16 && ip.indexOf('.') > 0) {
        #ifdef DEBUG_MODE
        Serial.print("Got IP: ");
        Serial.println(ip);
        #endif
        http.end();
        return ip;
      }
    }
    
    #ifdef DEBUG_MODE
    Serial.print("Service ");
    Serial.print(i + 1);
    Serial.print(" failed with code: ");
    Serial.println(httpCode);
    #endif
    
    http.end();
    delay(1000);
  }
  
  #ifdef DEBUG_MODE
  Serial.println("All IP check services failed");
  #endif
  return "";
}

/*
 * Update DNS record on Name.com
 */
bool updateDNSRecord(String ipAddress) {
  if (ipAddress.length() == 0) {
    #ifdef DEBUG_MODE
    Serial.println("Cannot update DNS: No IP address");
    #endif
    return false;
  }
  
  if (nameComUsername.length() == 0 || nameComApiToken.length() == 0) {
    #ifdef DEBUG_MODE
    Serial.println("Cannot update DNS: No API credentials");
    #endif
    return false;
  }
  
  if (domainName.length() == 0 || recordID.length() == 0) {
    #ifdef DEBUG_MODE
    Serial.println("Cannot update DNS: Domain or Record ID not configured");
    #endif
    return false;
  }
  
  HTTPClient http;
  String url = "https://api.name.com/v4/domains/" + domainName + "/records/" + recordID;
  
  #ifdef DEBUG_MODE
  Serial.print("Updating DNS record at: ");
  Serial.println(url);
  #endif
  
  http.begin(url);
  http.addHeader("Authorization", getBasicAuthHeader());
  http.addHeader("Content-Type", "application/json");
  
  DynamicJsonDocument doc(512);
  doc["host"] = recordHost;
  doc["type"] = "A";
  doc["answer"] = ipAddress;
  doc["ttl"] = ttlSeconds;
  
  String payload;
  serializeJson(doc, payload);
  
  #ifdef DEBUG_MODE
  Serial.print("Payload: ");
  Serial.println(payload);
  #endif
  
  int httpCode = http.PUT(payload);
  
  #ifdef DEBUG_MODE
  Serial.print("HTTP response code: ");
  Serial.println(httpCode);
  #endif
  
  if (httpCode == HTTP_CODE_OK) {
    String response = http.getString();
    #ifdef DEBUG_MODE
    Serial.print("Response: ");
    Serial.println(response);
    #endif
    
    DynamicJsonDocument respDoc(1024);
    DeserializationError error = deserializeJson(respDoc, response);
    
    if (!error) {
      if (respDoc.containsKey("answer") && respDoc["answer"].as<String>() == ipAddress) {
        #ifdef DEBUG_MODE
        Serial.println("DNS update successful!");
        #endif
        http.end();
        return true;
      }
    }
  } else if (httpCode > 0) {
    String response = http.getString();
    #ifdef DEBUG_MODE
    Serial.print("Error response: ");
    Serial.println(response);
    #endif
  } else {
    #ifdef DEBUG_MODE
    Serial.print("HTTP error: ");
    Serial.println(http.errorToString(httpCode).c_str());
    #endif
  }
  
  http.end();
  return false;
}

/*
 * Setup function
 */
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  
  #ifdef DEBUG_MODE
  Serial.println("\nESP32-C3 Name.com Dynamic DNS Updater");
  Serial.println("======================================");
  Serial.println("Version 2.0 - Secure Configuration");
  #endif
  
  // Configure GPIO trigger pin
  pinMode(CONFIG_BUTTON_PIN, INPUT_PULLDOWN);
  
  // Load configuration from NVS
  loadConfigFromNVS();
  
  // Check connection and start appropriate mode
  checkAndConnect();
  
  // Initial IP check and update (only if not in AP mode)
  if (wifiConnected && !apMode) {
    String ip = getExternalIP();
    if (ip.length() > 0) {
      currentExternalIP = ip;
      if (updateDNSRecord(ip)) {
        lastUpdateTime = millis();
        lastIPCheckTime = millis();
      }
    }
  }
}

/*
 * Main loop
 */
void loop() {
  // Handle web server and DNS server
  if (apMode) {
    dnsServer.processNextRequest();
  }
  server.handleClient();
  
  // Handle WiFi connection
  if (!apMode) {
    if (WiFi.status() != WL_CONNECTED) {
      wifiConnected = false;
      
      // Try to reconnect
      connectToWiFi();
      
      // If still not connected, start AP mode
      if (!wifiConnected) {
        startAPMode();
      }
    } else if (!wifiConnected) {
      wifiConnected = true;
      #ifdef DEBUG_MODE
      Serial.println("WiFi reconnected");
      #endif
    }
  }
  
  // DDNS update logic (only when connected to WiFi, not in AP mode)
  if (wifiConnected && !apMode) {
    unsigned long currentTime = millis();
    
    // Check IP periodically
    if (currentTime - lastIPCheckTime >= (updateIntervalMinutes * 60 * 1000)) {
      #ifdef DEBUG_MODE
      Serial.println("\nPerforming scheduled IP check...");
      #endif
      
      String newIP = getExternalIP();
      lastIPCheckTime = currentTime;
      
      if (newIP.length() > 0) {
        if (newIP != currentExternalIP || 
            currentTime - lastUpdateTime >= (updateIntervalMinutes * 60 * 1000)) {
          
          #ifdef DEBUG_MODE
          if (newIP != currentExternalIP) {
            Serial.print("IP changed from ");
            Serial.print(currentExternalIP);
            Serial.print(" to ");
            Serial.println(newIP);
          } else {
            Serial.println("Refreshing DNS record with same IP");
          }
          #endif
          
          if (updateDNSRecord(newIP)) {
            currentExternalIP = newIP;
            lastUpdateTime = currentTime;
          } else {
            #ifdef DEBUG_MODE
            Serial.println("DNS update failed, will retry later");
            #endif
            lastIPCheckTime = currentTime - ((updateIntervalMinutes * 60 * 1000) - (retryIntervalSeconds * 1000));
          }
        } else {
          #ifdef DEBUG_MODE
          Serial.println("IP unchanged, no update needed");
          #endif
        }
      } else {
        #ifdef DEBUG_MODE
        Serial.println("Failed to get external IP");
        #endif
        lastIPCheckTime = currentTime - ((updateIntervalMinutes * 60 * 1000) - (retryIntervalSeconds * 1000));
      }
    }
  }
  
  // Small delay to prevent watchdog issues
  delay(100);
}
