#!/usr/bin/env python3
"""Generate linAIx branding assets:
  1. toaru_logo.h  - 23x23 RGBA C header for sysinfo
  2. logo_login.png - login screen logo
"""
import struct, zlib, os

# ── 1. Generate toaru_logo.h (23×23 pixel "L" block logo in cyan) ──
# Design a stylized "L" made of colored blocks on 23x23 grid
# Using the same checkerboard-with-letter style as original
W = 23
H = 23

# Color: cyan/blue gradient like original (RGBA in file order: R,G,B,A)
# The original uses \0\260\377\377 = R=0, G=0xB0, B=0xFF, A=0xFF
# Let's make an "AI" monogram in cyan blocks

# Define the pixel grid: 1 = colored pixel, 0 = transparent
# Create "AI" text centered in 23x23
grid = [[0]*W for _ in range(H)]

# Letter patterns - we'll draw a blocky pixelated design
# Stylized "AI" in the center

# "A" shape (columns 3-10, rows 2-20)
a_pattern = [
    "  ####  ",
    " ##  ## ",
    "##    ##",
    "##    ##",
    "##    ##",
    "########",
    "##    ##",
    "##    ##",
    "##    ##",
]

# "I" shape (columns 13-19)
i_pattern = [
    "######",
    "  ##  ",
    "  ##  ",
    "  ##  ",
    "  ##  ",
    "  ##  ",
    "  ##  ",
    "  ##  ",
    "######",
]

# Place A at columns 1-8, rows centered
a_start_row = (H - len(a_pattern)*2) // 2
for py, row in enumerate(a_pattern):
    for px, ch in enumerate(row):
        if ch == '#':
            y1 = a_start_row + py * 2
            y2 = a_start_row + py * 2 + 1
            x = 1 + px
            if 0 <= x < W:
                if 0 <= y1 < H: grid[y1][x] = 1
                if 0 <= y2 < H: grid[y2][x] = 1

# Place I at columns 13-18
for py, row in enumerate(i_pattern):
    for px, ch in enumerate(row):
        if ch == '#':
            y1 = a_start_row + py * 2
            y2 = a_start_row + py * 2 + 1
            x = 13 + px
            if 0 <= x < W:
                if 0 <= y1 < H: grid[y1][x] = 1
                if 0 <= y2 < H: grid[y2][x] = 1

# Generate pixel data as C string
pixel_bytes = bytearray()
for y in range(H):
    green_base = 0xB2 - y * 5  # gradient like original
    if green_base < 0x40: green_base = 0x40
    for x in range(W):
        if grid[y][x]:
            # RGBA: R=0, G=gradient, B=0xFF, A=0xFF
            pixel_bytes.extend([0, green_base, 0xFF, 0xFF])
        else:
            pixel_bytes.extend([0, 0, 0, 0])

# Format as C string literals
lines = []
for i in range(0, len(pixel_bytes), 24):
    chunk = pixel_bytes[i:i+24]
    s = '"'
    for b in chunk:
        if b == 0:
            s += '\\0'
        elif b == 0xFF:
            s += '\\377'
        elif b == ord('\\'):
            s += '\\\\'
        elif b == ord('"'):
            s += '\\"'
        elif 32 <= b < 127:
            s += chr(b)
        else:
            s += '\\%o' % b
    s += '"'
    lines.append(s)

header_content = f"""/**
 * @file apps/toaru_logo.h
 * @brief linAIxOS logo - AI monogram
 *
 * Used by sysinfo. Can be used by other things as well.
 *
 * @copyright
 * This file is part of linAIxOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 */
static const struct {{
  unsigned int 	 width;
  unsigned int 	 height;
  unsigned int 	 bytes_per_pixel; /* 2:RGB16, 3:RGB, 4:RGBA */
  unsigned char	 pixel_data[{W} * {H} * 4 + 1];
}} gimp_image = {{
  {W}, {H}, 4,
  {chr(10).join('  ' + l for l in lines)}
}};
"""

script_dir = os.path.dirname(os.path.abspath(__file__))
with open(os.path.join(script_dir, 'apps', 'toaru_logo.h'), 'w', newline='\n') as f:
    f.write(header_content)
print("Generated apps/toaru_logo.h")


# ── 2. Generate logo_login.png (128×128 "LinAIx" text logo) ──
# We'll create a simple PNG with "LinAIx" text rendered in pixel font

IMG_W = 128
IMG_H = 128

# Define pixel font for "LinAIx" - each char is 5 wide x 7 tall in a grid
font = {
    'L': [
        "█    ",
        "█    ",
        "█    ",
        "█    ",
        "█    ",
        "█    ",
        "█████",
    ],
    'i': [
        "  █  ",
        "     ",
        " ██  ",
        "  █  ",
        "  █  ",
        "  █  ",
        " ███ ",
    ],
    'n': [
        "     ",
        "     ",
        "████ ",
        "█   █",
        "█   █",
        "█   █",
        "█   █",
    ],
    'A': [
        " ███ ",
        "█   █",
        "█   █",
        "█████",
        "█   █",
        "█   █",
        "█   █",
    ],
    'I': [
        "█████",
        "  █  ",
        "  █  ",
        "  █  ",
        "  █  ",
        "  █  ",
        "█████",
    ],
    'x': [
        "     ",
        "     ",
        "█   █",
        " █ █ ",
        "  █  ",
        " █ █ ",
        "█   █",
    ],
}

# Create RGBA image
pixels = bytearray(IMG_W * IMG_H * 4)

def set_pixel(px, py, r, g, b, a):
    if 0 <= px < IMG_W and 0 <= py < IMG_H:
        idx = (py * IMG_W + px) * 4
        pixels[idx] = r
        pixels[idx+1] = g
        pixels[idx+2] = b
        pixels[idx+3] = a

# Render "LinAIx" centered
text = "LinAIx"
char_w = 5
char_h = 7
scale = 3  # each font pixel = 3x3 real pixels
spacing = 1 * scale
total_w = len(text) * char_w * scale + (len(text) - 1) * spacing
start_x = (IMG_W - total_w) // 2
start_y = (IMG_H - char_h * scale) // 2

cursor_x = start_x
for ch in text:
    pattern = font[ch]
    for row_idx, row in enumerate(pattern):
        for col_idx, c in enumerate(row):
            if c == '█':
                for dy in range(scale):
                    for dx in range(scale):
                        px = cursor_x + col_idx * scale + dx
                        py = start_y + row_idx * scale + dy
                        # Cyan color with slight gradient
                        g = max(0x60, 0xB0 - row_idx * 10)
                        set_pixel(px, py, 0, g, 0xFF, 0xFF)
    cursor_x += char_w * scale + spacing

# Also draw a subtle border/glow
# (skipping for simplicity - the text alone looks clean)

# Write PNG manually (minimal PNG encoder)
def create_png(width, height, rgba_data):
    def chunk(chunk_type, data):
        c = chunk_type + data
        crc = struct.pack('>I', zlib.crc32(c) & 0xFFFFFFFF)
        return struct.pack('>I', len(data)) + c + crc

    # PNG signature
    sig = b'\x89PNG\r\n\x1a\n'

    # IHDR
    ihdr_data = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    ihdr = chunk(b'IHDR', ihdr_data)

    # IDAT - raw image data with filter bytes
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # filter: none
        row_start = y * width * 4
        raw.extend(rgba_data[row_start:row_start + width * 4])

    compressed = zlib.compress(bytes(raw), 9)
    idat = chunk(b'IDAT', compressed)

    # IEND
    iend = chunk(b'IEND', b'')

    return sig + ihdr + idat + iend

png_data = create_png(IMG_W, IMG_H, pixels)
png_path = os.path.join(script_dir, 'base', 'usr', 'share', 'logo_login.png')
with open(png_path, 'wb') as f:
    f.write(png_data)
print(f"Generated base/usr/share/logo_login.png ({len(png_data)} bytes)")

print("Done!")
