#!/usr/bin/env python3
"""Generate the 8 KB otadata images used by the web flasher.

The Xteink partition table has two app slots (ota_0 = app0 @0x10000,
ota_1 = app1 @0x780000) and an otadata record @0xe000 with two 32-byte
entries; the bootloader boots slot (seq - 1) % 2 of the valid entry with the
highest seq. Each image below writes BOTH entries, so exactly one is valid and
the choice is deterministic no matter what the device held before:

  otadata-boot-a.bin  entry0 seq=1 -> boots app0 (where the web flasher wrote HabitInk)
  otadata-boot-b.bin  entry1 seq=2 -> boots app1 (the stock reader / CrossPoint)

Usage: tools/make_otadata.py <out_dir>
"""
import pathlib, struct, sys, zlib

VALID = 2  # ESP_OTA_IMG_VALID

def entry(seq):
    e = struct.pack('<I', seq) + b'\xff' * 20 + struct.pack('<I', VALID)
    return e + struct.pack('<I', zlib.crc32(e[:4], 0xFFFFFFFF) & 0xFFFFFFFF)

def image(entry0, entry1):
    b = bytearray(b'\xff' * 0x2000)
    if entry0: b[0:32] = entry0
    if entry1: b[0x1000:0x1000 + 32] = entry1
    return bytes(b)

out = pathlib.Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
(out / 'otadata-boot-a.bin').write_bytes(image(entry(1), None))
(out / 'otadata-boot-b.bin').write_bytes(image(None, entry(2)))
print('wrote', out / 'otadata-boot-a.bin', out / 'otadata-boot-b.bin')
