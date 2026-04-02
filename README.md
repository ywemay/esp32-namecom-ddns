# ESP32-C3 Name.com Dynamic DNS Updater

This project runs on an ESP32-C3 module and periodically updates an A record on Name.com with the current external IP address of the internet connection.

## Features
- Periodically checks external IP using public IP services
- Updates Name.com DNS A record via API
- WiFi connection management with auto-reconnect
- Configurable update interval
- Serial debug output

## Hardware Requirements
- ESP32-C3 module
- USB cable for programming and power
- WiFi network access

## Configuration
1. Copy `config.example.h` to `config.h`
2. Edit `config.h` with your credentials:
   - WiFi SSID and password
   - Name.com API credentials
   - Domain and record details
   - Update interval

## Name.com API Setup
1. Log into your Name.com account
2. Go to Account Settings → API Access
3. Generate API token with DNS permissions
4. Use the token to create Basic Auth credentials

## Installation
1. Install Arduino IDE or PlatformIO
2. Install ESP32 board support
3. Clone this repository
4. Update configuration
5. Upload to ESP32-C3

## Usage
After uploading, open Serial Monitor at 115200 baud to see:
- WiFi connection status
- IP check results
- DNS update attempts and results
- Error messages if any

## API Reference
The project uses Name.com's v1 API:
- `PUT /v4/domains/{domain}/records/{id}` to update A record
- Basic Authentication with API token

## License
MIT