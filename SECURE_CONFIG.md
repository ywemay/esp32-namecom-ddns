# Secure Configuration System

## Overview

Version 2.0 introduces secure credential storage using the ESP32's built-in NVS (Non-Volatile Storage) system. No sensitive credentials are stored in source code files.

## Key Features

### 🔐 Secure Storage
- All credentials stored in encrypted NVS partition
- No hardcoded passwords or API tokens in source code
- Configuration survives power cycles and reboots

### 📱 Dual Configuration Modes

#### Access Point (AP) Mode
**Activates when:**
- No WiFi credentials configured (first run)
- GPIO2 pin pulled HIGH (emergency access)
- WiFi connection fails

**Characteristics:**
- No password required (user-friendly initial setup)
- Creates network: `ESP32-DDNS-XXXXXX` (unique per device)
- Captive portal redirects to configuration page
- Connect any device to configure

#### LAN Mode
**Activates when:**
- WiFi credentials are configured
- ESP32 successfully connects to WiFi network

**Characteristics:**
- Access web interface via ESP32's local IP address
- Password protection for configuration changes
- Status page with auto-refresh (30 seconds)

### 🔧 Hardware Trigger
- **GPIO2** pulled HIGH (connected to 3.3V) forces AP mode
- Useful for:
  - Emergency configuration access
  - Network changes
  - Factory recovery
- Can use a simple button or jumper wire

## Initial Setup

### First Boot
1. Power on ESP32
2. Look for WiFi network: `ESP32-DDNS-XXXXXX`
3. Connect to this network (no password)
4. Browser should auto-open to configuration page
5. If not, navigate to: `http://192.168.4.1`

### Configuration Steps
1. **WiFi Configuration**
   - Enter your WiFi SSID
   - Enter WiFi password
   - Click Save

2. **Name.com API**
   - Get API token from: https://www.name.com/account/settings/api
   - Enter username and API token
   - Click Save

3. **Domain Configuration**
   - Domain name (e.g., `example.com`)
   - Record host (`@` for root, or subdomain)
   - Record ID (use `get_record_id.py` script)
   - Click Save

4. **Security** (Recommended)
   - Set a configuration password
   - This password protects LAN-based changes
   - Click Save

5. Device will reboot and connect to WiFi

## Accessing Configuration Later

### Method 1: Via LAN (Recommended)
1. Find ESP32's IP address (check your router's DHCP table)
2. Navigate to `http://<esp32-ip-address>`
3. Enter configuration password (if set)
4. Modify settings as needed

### Method 2: Hardware Trigger
1. Connect GPIO2 to 3.3V (use a jumper or button)
2. Power on/reset ESP32
3. Connect to `ESP32-DDNS-XXXXXX` network
4. Navigate to `http://192.168.4.1`
5. Configure as needed
6. Remove jumper after saving

### Method 3: Serial Monitor
- View logs and status via serial (115200 baud)
- Useful for debugging

## Web Interface Pages

### `/` - Root
- Redirects to Config (AP mode) or Status (LAN mode)

### `/config` - Configuration
- Full configuration form
- All settings editable
- Password required in LAN mode

### `/status` - Status Dashboard
- Current IP address
- DDNS update status
- Network information
- Auto-refreshes every 30 seconds

### `/reset` - Factory Reset
- Warning page with confirmation
- Erases all credentials
- Returns to AP mode

## Security Considerations

### NVS Storage
- Credentials stored in NVS partition
- Not visible in source code
- Survives OTA updates (unless partition is wiped)

### Configuration Password
- Recommended for LAN mode
- Not required for initial AP setup (user-friendly)
- Protects against unauthorized changes on local network

### Physical Security
- GPIO2 trigger requires physical access
- Provides secure recovery option
- Consider adding a physical button for easy access

## Configuration Files

### `config.h`
Contains only:
- Default values (non-sensitive)
- GPIO pin definitions
- Constants and timeouts
- No credentials

### `config.example.h`
Template file with:
- Example values
- Documentation
- Safe to share publicly

## Factory Reset

### Via Web Interface
1. Navigate to `/reset`
2. Confirm warning
3. Click "Factory Reset"
4. Device restarts in AP mode

### Via Code (Development)
```cpp
preferences.begin("ddns_config", false);
preferences.clear();
preferences.end();
ESP.restart();
```

## Troubleshooting

### Can't Find AP Network
- Check power supply
- Verify ESP32 is booting (check LED if available)
- Try hardware trigger (GPIO2 to 3.3V)
- Check serial output for errors

### Can't Access Web Interface
- Ensure you're connected to correct network
- Try `http://192.168.4.1` (AP mode)
- Find ESP32 IP in router (LAN mode)
- Clear browser cache

### Configuration Not Saving
- Check NVS partition space
- Verify form submission
- Check serial logs for errors

### WiFi Connection Fails
- Verify SSID and password
- Check WiFi signal strength
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Try moving closer to router

## API Endpoints

| Endpoint | Method | Description | Password Required |
|----------|--------|-------------|-------------------|
| `/` | GET | Root (redirects) | No |
| `/config` | GET | Configuration page | LAN: Yes |
| `/save` | POST | Save configuration | LAN: Yes |
| `/status` | GET | Status dashboard | No |
| `/reset` | GET | Factory reset confirm | LAN: Yes |
| `/factory-reset` | POST | Execute factory reset | LAN: Yes |
| `/reboot` | POST | Reboot device | No |

## Migration from v1.x

### Before
```cpp
// config.h - INSECURE
#define WIFI_SSID "MyNetwork"
#define WIFI_PASSWORD "MyPassword"
#define NAME_COM_API_TOKEN "secret_token"
```

### After
```cpp
// config.h - SECURE
// No credentials here!
// Configure via web interface
#define DEFAULT_UPDATE_INTERVAL_MINUTES 10
```

### Migration Steps
1. Update `config.h` (credentials removed)
2. Flash new firmware
3. Configure via web interface
4. Old `config.h` credentials are ignored

## Technical Details

### NVS Namespace
- Namespace: `ddns_config`
- Stored keys:
  - `wifiSSID`, `wifiPassword`
  - `nameComUsername`, `nameComApiToken`
  - `domainName`, `recordHost`, `recordID`
  - `configPassword`
  - `updateInterval`, `retryInterval`, `ttl`

### Libraries Used
- `Preferences.h` - NVS access
- `WebServer.h` - HTTP server
- `DNSServer.h` - Captive portal DNS
- `WiFi.h` - Network connectivity

### Memory Usage
- NVS: ~500 bytes for configuration
- Web server: Dynamic (depends on page complexity)
- Minimum free heap: ~50KB recommended

## Best Practices

1. **Set a configuration password** after initial setup
2. **Note your Record ID** using the Python script before deployment
3. **Test AP mode** before deploying in final location
4. **Document your ESP32's MAC address** for easy identification
5. **Use strong WiFi password** (WPA2/WPA3)
6. **Consider physical security** of GPIO2 trigger
7. **Regular backups** of important configuration values

## Support

For issues or questions:
1. Check serial monitor output (115200 baud)
2. Review this documentation
3. Check ESP32 logs for specific errors
4. Verify network connectivity
