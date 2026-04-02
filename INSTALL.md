# Installation Guide

## Option 1: Using PlatformIO (Recommended)

### Prerequisites
1. Install [PlatformIO IDE](https://platformio.org/platformio-ide) or PlatformIO Core
2. Install [Python](https://www.python.org/) (required for PlatformIO)

### Steps
1. Clone or copy this project to your computer
2. Open the project folder in PlatformIO IDE
3. Update `config.example.h` with your credentials and save as `config.h`
4. Connect your ESP32-C3 via USB
5. Click the upload button (→) in PlatformIO
6. Open Serial Monitor to see output

## Option 2: Using Arduino IDE

### Prerequisites
1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Install ESP32 board support:
   - Open Arduino IDE
   - Go to File → Preferences
   - Add to Additional Boards Manager URLs: `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   - Go to Tools → Board → Boards Manager
   - Search for "esp32" and install "ESP32 by Espressif Systems"

### Steps
1. Copy all project files to a folder
2. Rename `config.example.h` to `config.h` and update with your credentials
3. Open `esp32_namecom_ddns.ino` in Arduino IDE
4. Select board: Tools → Board → ESP32 Arduino → "ESP32-C3 Dev Module"
5. Select correct port: Tools → Port
6. Install required libraries:
   - Sketch → Include Library → Manage Libraries
   - Search for "ArduinoJson" and install version 6.21.3 or later
7. Click Upload (→ button)
8. Open Serial Monitor (Tools → Serial Monitor) at 115200 baud

## Option 3: Using ESP-IDF (Advanced)

### Prerequisites
1. Install [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
2. Basic knowledge of ESP-IDF build system

### Steps
1. Create a new ESP-IDF project
2. Copy the source code files
3. Update configuration
4. Build and flash:
   ```bash
   idf.py set-target esp32c3
   idf.py menuconfig  # Configure WiFi and other settings
   idf.py build
   idf.py -p /dev/ttyUSB0 flash monitor
   ```

## Configuration Steps

### 1. Get Name.com API Credentials
1. Log into your [Name.com account](https://www.name.com/account/login)
2. Go to Account Settings → API Access
3. Click "Create Token"
4. Give it a name (e.g., "ESP32 DDNS")
5. Select permissions: "DNS: Read", "DNS: Write"
6. Copy the token (you won't see it again!)

### 2. Find Your DNS Record ID
Method A: Using the provided Python script:
```bash
# Install requests if needed
pip install requests

# Edit get_record_id.py with your credentials
python get_record_id.py
```

Method B: Using curl:
```bash
# Replace with your credentials
USERNAME="your_username"
API_TOKEN="your_token"
DOMAIN="yourdomain.com"

# Get all A records
curl -s -X GET \
  -H "Authorization: Basic $(echo -n "$USERNAME:$API_TOKEN" | base64)" \
  "https://api.name.com/v4/domains/$DOMAIN/records" | jq '.records[] | select(.type=="A")'
```

### 3. Update config.h
Copy `config.example.h` to `config.h` and update:
- `WIFI_SSID` and `WIFI_PASSWORD`: Your WiFi credentials
- `NAME_COM_USERNAME`: Your Name.com username
- `NAME_COM_API_TOKEN`: The API token you generated
- `DOMAIN_NAME`: Your domain (e.g., "example.com")
- `RECORD_HOST`: "@" for root domain or subdomain name
- `RECORD_ID`: The ID from step 2
- `UPDATE_INTERVAL_MINUTES`: How often to check (default: 10)

## Testing

### 1. Test API Access
Run the test script to verify everything works:
```bash
chmod +x test_api.sh
# Edit test_api.sh with your credentials first
./test_api.sh
```

### 2. Test ESP32 Connection
1. Upload the code to your ESP32-C3
2. Open Serial Monitor at 115200 baud
3. You should see:
   - WiFi connection status
   - External IP detection
   - DNS update attempts
   - Success or error messages

## Troubleshooting

### Common Issues

1. **WiFi won't connect**
   - Check SSID and password
   - Ensure ESP32 is in range
   - Check if WiFi network requires portal login

2. **API authentication fails**
   - Verify username and token
   - Check token has DNS read/write permissions
   - Try the test_api.sh script

3. **DNS update fails**
   - Verify RECORD_ID is correct
   - Check domain is registered with Name.com
   - Ensure record type is "A"

4. **Can't get external IP**
   - Check internet connection
   - Try changing IP_CHECK_SERVICE in config.h
   - Some networks block external IP services

5. **ESP32 won't upload**
   - Check USB cable (data cable required)
   - Press BOOT button while uploading if needed
   - Check correct port selected
   - Try different USB port

## Monitoring

After successful setup:
- The ESP32 will check IP every 10 minutes (configurable)
- Updates DNS only when IP changes
- Serial output shows all activities
- LED on ESP32 may blink during operations

## Security Notes

- Never commit `config.h` to version control
- Use strong API tokens with minimal permissions
- Consider using environment variables for sensitive data
- Regular updates help maintain security