#!/bin/bash
# Setup script for ESP32 Name.com DDNS project

set -e

echo "ESP32-C3 Name.com Dynamic DNS Setup"
echo "==================================="

# Check if config.h already exists
if [ -f "config.h" ]; then
    echo "config.h already exists. Backing up as config.h.backup"
    cp config.h config.h.backup
fi

# Copy example config
echo "Creating config.h from example..."
cp config.example.h config.h

echo ""
echo "Next steps:"
echo "1. Edit config.h with your credentials:"
echo "   - WiFi SSID and password"
echo "   - Name.com API credentials"
echo "   - Domain and record details"
echo ""
echo "2. Get your DNS record ID:"
echo "   python get_record_id.py"
echo ""
echo "3. Test API access:"
echo "   ./test_api.sh"
echo ""
echo "4. Upload to ESP32-C3:"
echo "   - Using PlatformIO: platformio run --target upload"
echo "   - Using Arduino IDE: Open esp32_namecom_ddns.ino and upload"
echo ""
echo "For detailed instructions, see INSTALL.md"