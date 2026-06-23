# Firmware Release Process

This document describes how to build and release firmware for PowerRelay.

## Automatic Release (Recommended)

### Prerequisites

- Git push access to the repository
- Tags enabled in GitHub Actions

### Steps

1. **Tag a new release**:
   ```bash
   git tag -a v1.0.0 -m "Release version 1.0.0"
   git push origin v1.0.0
   ```

2. **GitHub Actions workflow runs automatically**:
   - Builds firmware with ESP-IDF v6.0.1
   - Validates binary files
   - Creates GitHub Release with artifacts
   - Generates release notes automatically

3. **Artifacts included in release**:
   - `PowerRelay.bin` - Main application firmware
   - `bootloader.bin` - ESP32 bootloader
   - `partition-table.bin` - Partition table
   - `SHA256SUMS` - Checksums for verification
   - `FLASH_INSTRUCTIONS.txt` - Flashing guide

## Manual Release (via Workflow Dispatch)

1. Go to **GitHub Actions** → **Build and Release Firmware**
2. Click **Run workflow**
3. Check "Mark as pre-release" if applicable
4. Click **Run workflow**

The artifacts will be available in the workflow run.

## Local Build

### Prerequisites

- Python 3.11 or higher
- ESP-IDF v6.0.1
- esptool.py: `pip install esptool`

### Build Steps

```bash
# Setup ESP-IDF environment
source ~/esp/v6.0.1/esp-idf/export.sh

# Clean and build
idf.py fullclean
idf.py build

# Firmware is located at:
# - build/PowerRelay.bin
# - build/bootloader/bootloader.bin
# - build/partition_table/partition-table.bin
```

## Flashing the Firmware

### Quick Start

```bash
esptool.py -p /dev/ttyUSB0 -b 460800 --before default-reset --after hard-reset write_flash \
  0x1000 bootloader.bin \
  0x8000 partition-table.bin \
  0x10000 PowerRelay.bin
```

Replace `/dev/ttyUSB0` with your serial port (e.g., `COM3` on Windows).

### Complete Flash with Erase

```bash
# Erase entire flash
esptool.py -p /dev/ttyUSB0 erase_flash

# Flash all components
esptool.py -p /dev/ttyUSB0 -b 460800 write_flash \
  0x0 bootloader.bin \
  0x8000 partition-table.bin \
  0x10000 PowerRelay.bin
```

## Version Tagging Convention

Use semantic versioning for tags:

- `v1.0.0` - Major release (breaking changes)
- `v1.0.1` - Patch release (bug fixes)
- `v1.1.0` - Minor release (new features)
- `v1.0.0-rc1` - Release candidate

## Verification

### Check binary integrity

```bash
# Verify checksums (if SHA256SUMS file available)
sha256sum -c SHA256SUMS

# Or manually:
sha256sum *.bin
```

### Inspect binary headers

```bash
# Bootloader header
hexdump -C bootloader.bin | head -10

# App binary header
hexdump -C PowerRelay.bin | head -10
```

### Check binary with esptool

```bash
esptool.py image_info bootloader.bin
esptool.py image_info PowerRelay.bin
```

## Troubleshooting

### Build Fails

1. Ensure ESP-IDF v6.0.1 is installed
2. Clean build directory: `idf.py fullclean`
3. Check Python dependencies: `pip install -r $IDF_PATH/requirements.txt`

### Flash Fails

1. Try lower baud rate: `-b 115200` instead of `-b 460800`
2. Check USB drivers are installed
3. Try different USB cable or port
4. Put ESP32 in bootloader mode (hold BOOT button while powering on)

### Can't Download Release

1. Check GitHub Actions tab to see build status
2. Ensure tag is properly formatted: `v*`
3. Wait for GitHub Actions workflow to complete

## CI/CD Pipeline

The GitHub Actions workflow performs:

1. **Checkout** - Get latest code
2. **Setup** - Install Python 3.11 and ESP-IDF v6.0.1
3. **Build** - Compile firmware with `idf.py build`
4. **Validate** - Check binary files and headers
5. **Release** - Create GitHub Release with artifacts
6. **Notify** - Generate release summary

Status can be viewed in the **Actions** tab on GitHub.

## Release Checklist

Before tagging a release:

- [ ] Code is fully tested
- [ ] No uncommitted changes
- [ ] Version number updated if needed
- [ ] Changelog updated
- [ ] All tests pass
- [ ] Build completes locally
- [ ] Tag format is correct (v*.*.*)

## Automated Notifications

When a release is created:

- GitHub Release page is updated with artifacts
- Release notes are auto-generated from commit history
- Build summary is posted to workflow
- Artifacts are archived for 30 days

