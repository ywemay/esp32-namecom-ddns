// Configuration file - copy to config.h and update with your values

#ifndef CONFIG_H
#define CONFIG_H

// WiFi Configuration
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// Name.com API Configuration
// Get your API token from: https://www.name.com/account/settings/api
#define NAME_COM_USERNAME "your_username"  // Your Name.com username
#define NAME_COM_API_TOKEN "your_api_token"  // Your Name.com API token

// Domain Configuration
#define DOMAIN_NAME "yourdomain.com"  // Your domain registered with Name.com
#define RECORD_HOST "@"  // Use "@" for root domain or "subdomain" for subdomain
#define RECORD_ID "123456"  // Record ID from Name.com (get via API or web interface)

// Update Configuration
#define UPDATE_INTERVAL_MINUTES 10  // How often to check and update (in minutes)
#define RETRY_INTERVAL_SECONDS 30   // Retry interval on failure (in seconds)

// IP Check Services (fallback if one fails)
#define IP_CHECK_SERVICE_1 "https://api.ipify.org"
#define IP_CHECK_SERVICE_2 "https://checkip.amazonaws.com"
#define IP_CHECK_SERVICE_3 "https://ifconfig.me/ip"

// Debug Settings
#define SERIAL_BAUD_RATE 115200
#define DEBUG_MODE true  // Set to false to reduce serial output

#endif