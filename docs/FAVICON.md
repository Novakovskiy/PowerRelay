# Favicon Generation

This project uses an automated process to generate `favicon_data.h` from `favicon.ico`.

## How it works

1. **Source file**: `main/favicon.ico` - The original favicon image
2. **Generator script**: `scripts/generate_favicon_data.py` - Python script that converts .ico to .h
3. **Generated file**: `main/favicon_data.h` - Auto-generated header with binary data (Git-ignored)
4. **CMake integration**: Automatic generation during build process

## Manual regeneration

If you need to manually regenerate the favicon data (e.g., after updating the .ico file):

```bash
python scripts/generate_favicon_data.py main/favicon.ico main/favicon_data.h
```

## Updating the favicon

To update the favicon:

1. Replace `main/favicon.ico` with a new 32x32 pixel .ico file
2. Run `idf.py build` - it will automatically regenerate `favicon_data.h`
3. The favicon will be served by the HTTP server

## Requirements

- Python 3.x (already required by ESP-IDF)
- The .ico file should be 32x32 pixels (as used in the current implementation)

