#!/usr/bin/env python3
"""
Generate C++ header with embedded favicon binary data from ICO file.
Usage: python generate_favicon_data.py <input_favicon.ico> <output_favicon_data.h>
"""

import sys
import os
from pathlib import Path


def generate_favicon_header(input_file: str, output_file: str) -> None:
    """Generate C++ header with binary favicon data."""

    # Read binary data from ico file
    with open(input_file, 'rb') as f:
        data = f.read()

    # Generate C++ code
    header_content = f"""#pragma once

#include <cstdint>

// Generated from {Path(input_file).name}
const uint8_t FAVICON_ICO[] = {{
"""

    # Add bytes in rows of 16
    bytes_per_row = 16
    for i in range(0, len(data), bytes_per_row):
        row = data[i:i+bytes_per_row]
        hex_bytes = ','.join(f'0x{b:02X}' for b in row)
        header_content += f"\t{hex_bytes}"

        # Add comma and newline, except for last row
        if i + bytes_per_row < len(data):
            header_content += ",\n"
        else:
            header_content += "\n"

    header_content += f"""}}; // Size: {len(data)} bytes
"""

    # Write to output file
    with open(output_file, 'w') as f:
        f.write(header_content)

    print(f"✓ Generated {output_file} ({len(data)} bytes)")


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input_favicon.ico> <output_favicon_data.h>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]

    if not os.path.exists(input_file):
        print(f"✗ Input file not found: {input_file}")
        sys.exit(1)

    try:
        generate_favicon_header(input_file, output_file)
    except Exception as e:
        print(f"✗ Error: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()

