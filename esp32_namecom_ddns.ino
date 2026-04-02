/*
  ESP32-C3 Name.com Dynamic DNS Updater
  Updates an A record on Name.com with the current external IP address
  
  Requirements:
  - WiFi connection
  - Name.com API credentials
  - Domain registered with Name.com
  
  API Documentation: https://www.name.com/api-docs
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Include configuration
#include "config.h"

// Global variables
String currentExternalIP = "";
unsigned long lastUpdateTime = 0;
unsigned long lastIPCheckTime = 0;
bool wifiConnected = false;

// Base64 encoding function for Basic Auth
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

// Get Basic Auth header value
String getBasicAuthHeader() {
  String credentials = String(NAME_COM_USERNAME) + ":" + String(NAME_COM_API_TOKEN);
  return "Basic " + base64Encode(credentials);
}

// Connect to WiFi
void connectToWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    return;
  }
  
  #ifdef DEBUG_MODE
  Serial.println("Connecting to WiFi...");
  #endif
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
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

// Get external IP address from a service
String getExternalIP() {
  HTTPClient http;
  String ip = "";
  
  // Try multiple services in case one fails
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
      
      // Validate IP format (basic check)
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

// Update DNS record on Name.com
bool updateDNSRecord(String ipAddress) {
  if (ipAddress.length() == 0) {
    #ifdef DEBUG_MODE
    Serial.println("Cannot update DNS: No IP address");
    #endif
    return false;
  }
  
  HTTPClient http;
  String url = "https://api.name.com/v4/domains/" + String(DOMAIN_NAME) + "/records/" + String(RECORD_ID);
  
  #ifdef DEBUG_MODE
  Serial.print("Updating DNS record at: ");
  Serial.println(url);
  #endif
  
  http.begin(url);
  http.addHeader("Authorization", getBasicAuthHeader());
  http.addHeader("Content-Type", "application/json");
  
  // Create JSON payload
  DynamicJsonDocument doc(512);
  doc["host"] = RECORD_HOST;
  doc["type"] = "A";
  doc["answer"] = ipAddress;
  doc["ttl"] = 300;  // 5 minutes TTL
  
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
    
    // Parse response to verify update
    DynamicJsonDocument respDoc(1024);
    DeserializationError error = deserializeJson(respDoc, response);
    
    if (!error) {
      if (respDoc.containsKey("answer") && respDoc["answer"] == ipAddress) {
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

// Setup function
void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  
  #ifdef DEBUG_MODE
  Serial.println("\nESP32-C3 Name.com Dynamic DNS Updater");
  Serial.println("======================================");
  #endif
  
  connectToWiFi();
  
  // Initial IP check and update
  if (wifiConnected) {
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

// Main loop
void loop() {
  unsigned long currentTime = millis();
  
  // Handle WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    connectToWiFi();
  } else if (!wifiConnected) {
    wifiConnected = true;
    #ifdef DEBUG_MODE
    Serial.println("WiFi reconnected");
    #endif
  }
  
  if (wifiConnected) {
    // Check IP periodically
    if (currentTime - lastIPCheckTime >= (UPDATE_INTERVAL_MINUTES * 60 * 1000)) {
      #ifdef DEBUG_MODE
      Serial.println("\nPerforming scheduled IP check...");
      #endif
      
      String newIP = getExternalIP();
      lastIPCheckTime = currentTime;
      
      if (newIP.length() > 0) {
        // Update DNS if IP changed or it's time for a refresh
        if (newIP != currentExternalIP || 
            currentTime - lastUpdateTime >= (UPDATE_INTERVAL_MINUTES * 60 * 1000)) {
          
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
            // Schedule retry sooner
            lastIPCheckTime = currentTime - ((UPDATE_INTERVAL_MINUTES * 60 * 1000) - (RETRY_INTERVAL_SECONDS * 1000));
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
        // Schedule retry sooner
        lastIPCheckTime = currentTime - ((UPDATE_INTERVAL_MINUTES * 60 * 1000) - (RETRY_INTERVAL_SECONDS * 1000));
      }
    }
  }
  
  // Small delay to prevent watchdog issues
  delay(1000);
}