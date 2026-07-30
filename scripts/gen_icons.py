#!/usr/bin/env python3
"""Generate the HabitInk 1-bit icon set.

Each habit carries an icon token (see habitcore/IconSet.h). The firmware and the
companion both render a small monochrome glyph for it. This script draws each
icon at 3x resolution with Pillow's vector primitives, downsamples to a crisp
1-bit 24x24 bitmap, and emits a committed C++ source (no runtime Pillow
dependency). Icon bit convention: a set bit means "draw ink".

Run from the repo root:

    python3 scripts/gen_icons.py

Writes lib/HabitUI/src/Icons.cpp. The icon order matches habitcore::IconId.
"""
from __future__ import annotations

import math

from PIL import Image, ImageDraw

SIZE = 24
SS = 3  # supersample factor
S = SIZE * SS

# Order MUST match habitcore::IconId.
TOKENS = [
    "check", "water", "run", "book", "pill", "meditate", "dumbbell", "sun",
    "moon", "heart", "food", "pen", "code", "music", "leaf", "star",
]


def new_canvas():
    img = Image.new("L", (S, S), 0)
    return img, ImageDraw.Draw(img)


def line(d, pts, w):
    d.line([(x * SS, y * SS) for x, y in pts], fill=255, width=w * SS, joint="curve")


def poly(d, pts, fill=True):
    scaled = [(x * SS, y * SS) for x, y in pts]
    d.polygon(scaled, fill=255 if fill else 0, outline=255)


def ellipse(d, box, w=0, fill=False):
    b = [c * SS for c in box]
    d.ellipse(b, outline=255, width=max(1, w * SS), fill=255 if fill else None)


def draw_check(d):
    line(d, [(4, 13), (10, 19), (20, 5)], 3)


def draw_water(d):
    # Teardrop: triangle top + circle bottom.
    poly(d, [(12, 3), (5, 15), (19, 15)])
    ellipse(d, (5, 9, 19, 21), fill=True)


def draw_run(d):
    ellipse(d, (12, 2, 18, 8), fill=True)          # head
    line(d, [(15, 8), (11, 14)], 2)                 # torso
    line(d, [(11, 14), (5, 12)], 2)                 # back arm
    line(d, [(11, 14), (17, 13)], 2)                # front arm
    line(d, [(11, 14), (8, 22)], 2)                 # back leg
    line(d, [(11, 14), (17, 20)], 2)                # front leg


def draw_book(d):
    line(d, [(12, 5), (12, 20)], 2)                 # spine
    poly(d, [(12, 5), (4, 6), (4, 19), (12, 20)], fill=False)
    poly(d, [(12, 5), (20, 6), (20, 19), (12, 20)], fill=False)
    line(d, [(12, 5), (4, 6)], 1)
    line(d, [(4, 6), (4, 19)], 1)
    line(d, [(4, 19), (12, 20)], 1)
    line(d, [(12, 5), (20, 6)], 1)
    line(d, [(20, 6), (20, 19)], 1)
    line(d, [(20, 19), (12, 20)], 1)


def draw_pill(d):
    # Capsule at 45 degrees.
    line(d, [(6, 18), (18, 6)], 9)
    # Divider.
    line(d, [(9, 15), (15, 9)], 1)


def draw_meditate(d):
    ellipse(d, (9, 2, 15, 8), fill=True)            # head
    poly(d, [(12, 9), (4, 20), (20, 20)])           # body triangle
    line(d, [(4, 18), (9, 15)], 2)                  # arms
    line(d, [(20, 18), (15, 15)], 2)


def draw_dumbbell(d):
    line(d, [(7, 12), (17, 12)], 2)                 # bar
    d.rectangle([3 * SS, 7 * SS, 6 * SS, 17 * SS], fill=255)
    d.rectangle([18 * SS, 7 * SS, 21 * SS, 17 * SS], fill=255)


def draw_sun(d):
    ellipse(d, (8, 8, 16, 16), fill=True)
    for a in range(0, 360, 45):
        r = math.radians(a)
        x0 = 12 + 8 * math.cos(r)
        y0 = 12 + 8 * math.sin(r)
        x1 = 12 + 11 * math.cos(r)
        y1 = 12 + 11 * math.sin(r)
        line(d, [(x0, y0), (x1, y1)], 2)


def draw_moon(d):
    big = Image.new("L", (S, S), 0)
    bd = ImageDraw.Draw(big)
    bd.ellipse([4 * SS, 3 * SS, 20 * SS, 21 * SS], fill=255)
    cut = Image.new("L", (S, S), 0)
    cd = ImageDraw.Draw(cut)
    cd.ellipse([9 * SS, 1 * SS, 25 * SS, 19 * SS], fill=255)
    big.paste(0, (0, 0), cut)
    d.bitmap((0, 0), big, fill=255)


def draw_heart(d):
    ellipse(d, (5, 5, 13, 13), fill=True)
    ellipse(d, (11, 5, 19, 13), fill=True)
    poly(d, [(5, 10), (19, 10), (12, 21)])


def draw_food(d):
    # Apple: body + stem + leaf.
    ellipse(d, (5, 7, 19, 21), fill=True)
    line(d, [(12, 7), (12, 3)], 1)                  # stem
    poly(d, [(12, 5), (16, 3), (15, 6)])            # leaf


def draw_pen(d):
    poly(d, [(5, 20), (7, 15), (16, 6), (19, 9), (10, 18)])  # body
    poly(d, [(5, 20), (7, 17), (8, 19)])            # nib
    line(d, [(16, 6), (19, 9)], 1)


def draw_code(d):
    line(d, [(9, 6), (4, 12), (9, 18)], 2)          # <
    line(d, [(15, 6), (20, 12), (15, 18)], 2)       # >


def draw_music(d):
    ellipse(d, (5, 15, 11, 21), fill=True)          # note head
    line(d, [(11, 18), (11, 5)], 2)                 # stem
    line(d, [(11, 5), (18, 7)], 2)                  # flag
    line(d, [(11, 9), (18, 11)], 2)


def draw_leaf(d):
    poly(d, [(5, 19), (9, 6), (19, 5), (18, 15), (5, 19)])
    line(d, [(7, 17), (17, 7)], 1)                  # vein


def draw_star(d):
    pts = []
    for i in range(10):
        ang = math.radians(-90 + i * 36)
        rad = 10 if i % 2 == 0 else 4
        pts.append((12 + rad * math.cos(ang), 12 + rad * math.sin(ang)))
    poly(d, pts)


DRAWERS = {
    "check": draw_check, "water": draw_water, "run": draw_run, "book": draw_book,
    "pill": draw_pill, "meditate": draw_meditate, "dumbbell": draw_dumbbell,
    "sun": draw_sun, "moon": draw_moon, "heart": draw_heart, "food": draw_food,
    "pen": draw_pen, "code": draw_code, "music": draw_music, "leaf": draw_leaf,
    "star": draw_star,
}


def render(token: str) -> list[list[int]]:
    img, d = new_canvas()
    DRAWERS[token](d)
    small = img.resize((SIZE, SIZE), Image.LANCZOS)
    px = small.load()
    rows = []
    for y in range(SIZE):
        row = [1 if px[x, y] >= 110 else 0 for x in range(SIZE)]
        rows.append(row)
    return rows


def pack_row(row: list[int]) -> list[int]:
    out = []
    for byte_start in range(0, SIZE, 8):
        b = 0
        for bit in range(8):
            x = byte_start + bit
            if x < SIZE and row[x]:
                b |= 0x80 >> bit
        out.append(b)
    return out


def main() -> None:
    stride = (SIZE + 7) // 8
    lines = []
    lines.append("// GENERATED by scripts/gen_icons.py -- do not edit by hand.")
    lines.append("//")
    lines.append("// %dx%d 1-bit icons, row-major, MSB = leftmost pixel, set bit = ink." % (SIZE, SIZE))
    lines.append("// Order matches habitcore::IconId.")
    lines.append('#include "habitui/Icons.h"')
    lines.append("")
    lines.append("namespace habitui {")
    lines.append("namespace {")
    for token in TOKENS:
        rows = render(token)
        packed = []
        for row in rows:
            packed.extend(pack_row(row))
        name = "kIcon_" + token
        body = ", ".join("0x%02X" % b for b in packed)
        lines.append("const uint8_t %s[%d] = {%s};" % (name, stride * SIZE, body))
    lines.append("")
    lines.append("const IconBitmap kIcons[] = {")
    for token in TOKENS:
        lines.append("    {%d, kIcon_%s}," % (SIZE, token))
    lines.append("};")
    lines.append("}  // namespace")
    lines.append("")
    lines.append("const IconBitmap& iconBitmap(habitcore::IconId id) {")
    lines.append("  int i = static_cast<int>(id);")
    lines.append("  if (i < 0 || i >= habitcore::kIconCount) i = 0;")
    lines.append("  return kIcons[i];")
    lines.append("}")
    lines.append("")
    lines.append("}  // namespace habitui")
    lines.append("")

    out_path = "lib/HabitUI/src/Icons.cpp"
    with open(out_path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(lines))
    print("wrote", out_path, "with", len(TOKENS), "icons")


if __name__ == "__main__":
    main()
