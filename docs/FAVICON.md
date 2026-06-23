# Favicon Management

This project includes a favicon that is embedded into the firmware as binary data.

## Current approach

- **Source file**: `main/favicon.ico` - The original favicon image (32x32 pixels)
- **Generated file**: `main/favicon_data.h` - C++ header with binary data (kept in git)
- **Generator script**: `scripts/generate_favicon_data.py` - Python script for manual regeneration

The `favicon_data.h` file is kept in git for build stability. It's generated from `favicon.ico` using the Python script.

## Updating the favicon

To update the favicon:

1. Replace `main/favicon.ico` with a new 32x32 pixel .ico file
2. Manually regenerate the header:
   ```bash
   python scripts/generate_favicon_data.py main/favicon.ico main/favicon_data.h
   ```
3. Commit both `favicon.ico` and `favicon_data.h`
4. Run `idf.py build` to compile the firmware

## Manual regeneration

If you need to manually regenerate the favicon data:

```bash
python scripts/generate_favicon_data.py main/favicon.ico main/favicon_data.h
```

The script will:
- Read the binary data from `favicon.ico`
- Generate a C++ header file with the binary data as a uint8_t array
- Output status messages

## Requirements

- Python 3.x
- .ico file should be 32x32 pixels (as used in the current implementation)

## Notes

- The binary size of the favicon is approximately 4.3 KB
- It's embedded directly in the firmware, no external file needed
- The favicon is served via HTTP on the web interface


