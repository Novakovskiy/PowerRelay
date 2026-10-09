# PowerRelay - ESP32 Relay Control

A web-based relay control device powered by ESP32, with MQTT support and web interface.

## Features

- ✅ **WiFi Connectivity** - Both Station and Access Point modes
- ✅ **Web Interface** - Control relay via HTTP(S) web page
- ✅ **MQTT Support** - Remote control via MQTT protocol
- ✅ **Home Assistant Discovery** - Auto-discovery for Home Assistant
- ✅ **Configuration Dashboard** - Customize settings via web interface
- ✅ **Status LED** - Visual feedback of device status
- ✅ **Hardware Button** - Physical power button control
- ✅ **UART Control** - Command-line interface (optional)
- ✅ **Favicon Embedded** - Custom favicon in firmware

## Hardware Requirements

- ESP32 development board
- 5V relay module
- USB-A to Micro-USB cable
- WiFi network (2.4GHz)

### GPIO Configuration

| Pin | Function | Notes |
|-----|----------|-------|
| GPIO 0 | BOOT Button | Press 10s to factory reset |
| GPIO 2 | Relay Output | Controls main relay |
| GPIO 4 | Status LED | Indicates connection status |

## Building

### Prerequisites

```bash
# Install ESP-IDF 6.0.1
# https://github.com/espressif/esp-idf/releases/v6.0.1

# Install Python dependencies
pip install esptool
```

### Build Steps

```bash
# Setup ESP-IDF environment
source ~/esp/v6.0.1/esp-idf/export.sh  # Linux/Mac
# or on Windows
C:\esp\v6.0.1\esp-idf\export.bat

# Build firmware
idf.py build

# Clean if needed
idf.py fullclean
```

## Flashing

### Quick Start

```bash
idf.py -p /dev/ttyUSB0 flash
```

### Manual Flash with esptool

```bash
esptool.py -p /dev/ttyUSB0 -b 460800 --before default-reset --after hard-reset write_flash \
  0x1000 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/PowerRelay.bin
```

Replace `/dev/ttyUSB0` with your serial port (e.g., `COM3` on Windows).

## Configuration

### Via Web Interface

1. Connect to the device's WiFi network (SSID: "Power Relay" by default)
2. Open browser to `http://192.168.4.1`
3. Navigate to **Settings** tab
4. Configure:
   - WiFi SSID and Password
   - Device Hostname
   - MQTT settings (if needed)
   - UART control options

### Via MQTT

**Topics**:

- `power_relay/set` - Send "ON", "OFF", or "TOGGLE"
- `power_relay/state` - Publishes current state ("ON" or "OFF")
- `power_relay/availability` - Publishes "online" or "offline"

**Example** (using mosquitto_pub):

```bash
mosquitto_pub -h mqtt.example.com -t power_relay/set -m "ON"
```

### UART Control Behavior

When **Use UART control** is enabled in Settings:

- On relay ON, firmware sends `power_on` over UART.
- On relay OFF request, firmware sends `power_off`, waits for **Delay before shutdown**, then switches relay output off.
- Firmware listens for UART commands and applies the same ON/OFF logic for incoming `power_on` and `power_off`.
- During this shutdown delay, MQTT availability is set to `offline`, so Home Assistant shows the entity as temporarily unavailable and blocks toggling until timeout completes.

## API Endpoints

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/` | GET | Main web interface |
| `/settings` | GET | Settings page |
| `/api/settings` | GET | Get configuration (JSON) |
| `/api/settings` | POST | Update configuration |
| `/api/status` | GET | Get current relay status (JSON) |
| `/action?cmd=on` | POST | Toggle relay |
| `/api/reboot` | POST | Reboot device |
| `/api/reset` | POST | Factory reset |

## Development

### Code Structure

```
main/
├── PowerRelay.cpp         # Main application
├── web_page.h             # Web interface HTML/JS
├── favicon_data.h         # Embedded favicon
├── CMakeLists.txt         # Build configuration
└── favicon.ico            # Source favicon

scripts/
└── generate_favicon_data.py  # Favicon generator

docs/
├── FAVICON.md             # Favicon management
├── RELEASE.md             # Release process
└── README.md              # This file

.github/workflows/
└── release-firmware.yml   # GitHub Actions CI/CD
```

### Modifying Code

1. Edit `main/PowerRelay.cpp` for main functionality
2. Edit `main/web_page.h` for web interface
3. Update favicon: Replace `main/favicon.ico` and regenerate
4. Build and test: `idf.py build`

### Code Quality

- ✅ clang-tidy checks (all passing)
- ✅ No compilation warnings
- ✅ constexpr optimizations
- ✅ Raw string literals for JSON

## Release Process

### Creating a Release

```bash
# Create and push a tag
git tag -a v1.0.0 -m "Release version 1.0.0"
git push origin v1.0.0

# GitHub Actions will automatically build and create release
```

See [docs/RELEASE.md](docs/RELEASE.md) for detailed instructions.

### Download Releases

Latest releases available at: https://github.com/YOUR_USERNAME/PowerRelay/releases

## Troubleshooting

### Can't Connect to Web Interface

1. Ensure device is powered
2. Check WiFi network: "Power Relay" (if in AP mode)
3. Connect to `http://192.168.4.1` (Access Point mode)
4. Or check your router for device IP (Station mode)

### MQTT Connection Fails

1. Verify MQTT broker address is correct
2. Check firewall isn't blocking MQTT port (1883)
3. Verify username/password if required
4. Check logs via serial console

### Relay Won't Turn On/Off

1. Verify GPIO pin configuration (GPIO 2)
2. Check relay module wiring
3. Ensure 5V power supply to relay
4. Test with web interface or MQTT

### Device Keeps Rebooting

1. Check for corrupted filesystem: `idf.py erase_flash`
2. Flash bootloader again: `idf.py -p PORT erase_flash flash`
3. Check power supply voltage stability

## Serial Console

Connect via USB serial to view logs:

```bash
# Using screen
screen /dev/ttyUSB0 115200

# Using minicom
minicom -D /dev/ttyUSB0 -b 115200

# Using PuTTY
# Set port to COMx, baud 115200
```

## Performance

### Binary Sizes

- **Bootloader**: ~26 KB
- **Partition Table**: <1 KB  
- **Application**: ~930 KB
- **Total Flash Used**: ~37% of 4MB

### Memory Usage

- **RAM**: ~300-400 KB available
- **PSRAM**: Not used

## License

[Add your license here]

## Contributing

1. Fork the repository
2. Create feature branch: `git checkout -b feature/new-feature`
3. Commit changes: `git commit -am 'Add new feature'`
4. Push to branch: `git push origin feature/new-feature`
5. Create Pull Request

## Support

- 🐛 Report bugs: [GitHub Issues](https://github.com/YOUR_USERNAME/PowerRelay/issues)
- 💡 Suggest features: [GitHub Discussions](https://github.com/YOUR_USERNAME/PowerRelay/discussions)
- 📚 Documentation: [Docs folder](docs/)

## Changelog

See [GitHub Releases](https://github.com/YOUR_USERNAME/PowerRelay/releases) for version history.

---

**Last Updated**: 2026-06-24  
**ESP-IDF Version**: 6.0.1  
**Target Device**: ESP32
