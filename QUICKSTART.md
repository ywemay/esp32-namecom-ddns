# Quick Start Guide - ESP32 DDNS v2.0

## 5-Minute Setup

### Step 1: Flash Firmware (2 minutes)
```bash
# Using Arduino IDE
1. Open esp32_namecom_ddns.ino in Arduino IDE
2. Select board: "ESP32C3 Dev Module"
3. Select port: /dev/ttyACM0 (or your ESP32 port)
4. Click Upload

# Or using platformio
cd /home/dorian/Arduino/esp32_namecom_ddns
pio run --target upload
```

### Step 2: Initial Configuration (2 minutes)
1. **Power on** the ESP32
2. **Find WiFi network**: `ESP32-DDNS-XXXXXX` (unique to your device)
3. **Connect** to this network (no password needed)
4. **Open browser** - should auto-redirect to config page
   - If not, go to: `http://192.168.4.1`

### Step 3: Fill Out Form (1 minute)

#### WiFi Section
- **SSID**: Your home WiFi name
- **Password**: Your WiFi password

#### Name.com API Section
- Get token from: https://www.name.com/account/settings/api
- **Username**: Your Name.com username
- **API Token**: Paste the token

#### Domain Section
- **Domain**: `yourdomain.com`
- **Record Host**: `@` (for root) or `www` (for subdomain)
- **Record ID**: Run this command first:
  ```bash
  python3 get_record_id.py yourdomain.com @
  ```

#### Security (Recommended)
- **Config Password**: Choose a password (remember it!)

#### Click "Save Configuration"

### Step 4: Done! ✅
- Device will reboot
- Connect to your WiFi
- Update DNS record automatically
- Access status at: `http://<esp32-ip-address>`

---

## Find ESP32 IP Address

### Method 1: Router Admin
1. Log into your router (usually 192.168.1.1)
2. Look for DHCP client list
3. Find "ESP32" or similar device
4. Note the IP address

### Method 2: Serial Monitor
1. Open Arduino IDE Serial Monitor
2. Set baud rate: 115200
3. Look for: `IP address: 192.168.x.x`

### Method 3: Network Scanner
```bash
# Install nmap
sudo apt install nmap

# Scan your network
nmap -sn 192.168.1.0/24

# Look for Espressif MAC addresses
```

---

## Access Web Interface

### Once Connected to WiFi
1. Find ESP32's IP (see above)
2. Open browser: `http://<ip-address>`
3. Enter config password (if you set one)
4. View status or change settings

### Status Page Shows
- Current external IP
- Last update time
- WiFi signal strength
- Network details
- Configuration summary

---

## Troubleshooting

### Can't Find ESP32 Network?
- Check power (USB cable, power adapter)
- Wait 30 seconds for boot
- Try pressing reset button
- Check serial output for errors

### Can't Connect to WiFi?
- Verify SSID and password (case-sensitive!)
- Ensure 2.4GHz network (ESP32 doesn't do 5GHz)
- Move closer to router
- Check for special characters in password

### Web Page Won't Load?
- Ensure you're on same network as ESP32
- Try `http://` not `https://`
- Clear browser cache
- Try different browser/device

### DNS Not Updating?
- Check Name.com credentials
- Verify Record ID is correct
- Check serial monitor for error messages
- Ensure domain is registered with Name.com

---

## Advanced: Hardware Trigger

Need to reconfigure but can't access LAN?

### Force AP Mode
1. **Connect GPIO2 to 3.3V**
   - Use a jumper wire or button
   - GPIO2 pin → 3.3V pin
2. **Power on/reset ESP32**
3. **Connect to** `ESP32-DDNS-XXXXXX`
4. **Reconfigure** via web interface
5. **Remove jumper** when done

### Add a Button (Optional)
```
GPIO2 ──[10kΩ]── GND
GPIO2 ──[Button]── 3.3V
```
Press button during boot to force AP mode.

---

## Factory Reset

### Via Web Interface
1. Go to `/reset` or click "Factory Reset"
2. Confirm warning
3. Device resets to AP mode

### Via Code (Last Resort)
```cpp
// Add to setup() temporarily
#include <Preferences.h>
Preferences preferences;
preferences.begin("ddns_config", false);
preferences.clear();
preferences.end();
ESP.restart();
```

---

## Next Steps

### Recommended
- [ ] Set a strong configuration password
- [ ] Document your ESP32's IP address
- [ ] Test that DNS updates correctly
- [ ] Add a physical button for GPIO2 trigger
- [ ] Mount ESP32 in a safe location

### Optional
- [ ] Enable debug mode for monitoring
- [ ] Set up email notifications (future feature)
- [ ] Integrate with home automation
- [ ] Add HTTPS support (advanced)

---

## Support Files

- `SECURE_CONFIG.md` - Detailed documentation
- `CHANGES_v2.md` - What's new in v2.0
- `INSTALL.md` - Original installation guide
- `README.md` - Project overview

---

## Quick Reference

| Action | How To |
|--------|--------|
| Initial setup | Connect to ESP32-DDNS-XXXXXX |
| Change settings | http://\<esp32-ip\>/config |
| View status | http://\<esp32-ip\>/status |
| Force AP mode | GPIO2 to 3.3V |
| Factory reset | Web interface → Reset |
| Find IP | Check router or serial monitor |
| Get Record ID | `python3 get_record_id.py` |
| Debug output | Serial Monitor @ 115200 |

---

**Need Help?** Check serial monitor output (115200 baud) for detailed logs.
