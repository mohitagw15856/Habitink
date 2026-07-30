#!/usr/bin/env python3
"""Assemble the rendered PBM frames into the README demo GIF.

Gives the 1-bit frames a soft "e-ink on paper" look (warm off-white paper,
near-black ink, a thin device bezel) and writes an animated GIF. Run after
render_frames has produced the PBM sequence.

    python3 scripts/make_gifs.py <frames_dir> docs/images/demo.gif
"""
from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image

PAPER = (244, 243, 236)
INK = (26, 26, 26)
BEZEL = (32, 34, 38)
BEZEL_PAD = 22
RADIUS = 26
SCALE = 1  # frames are already 800x480


def stylise(frame: Image.Image) -> Image.Image:
    frame = frame.convert("1")
    w, h = frame.size
    # Map 1-bit to the paper/ink palette.
    screen = Image.new("RGB", (w, h), PAPER)
    px = frame.load()
    sp = screen.load()
    for y in range(h):
        for x in range(w):
            if px[x, y] == 0:  # ink
                sp[x, y] = INK
    # Rounded device bezel around the screen.
    canvas = Image.new("RGB", (w + 2 * BEZEL_PAD, h + 2 * BEZEL_PAD), BEZEL)
    mask = Image.new("L", canvas.size, 0)
    from PIL import ImageDraw

    ImageDraw.Draw(mask).rounded_rectangle([0, 0, canvas.size[0] - 1, canvas.size[1] - 1], radius=RADIUS, fill=255)
    canvas.paste(screen, (BEZEL_PAD, BEZEL_PAD))
    out = Image.new("RGB", canvas.size, (255, 255, 255))
    out.paste(canvas, (0, 0), mask)
    return out


def main() -> None:
    frames_dir = Path(sys.argv[1] if len(sys.argv) > 1 else "/tmp/frames_out")
    out_path = Path(sys.argv[2] if len(sys.argv) > 2 else "docs/images/demo.gif")

    pbms = sorted(frames_dir.glob("frame_*.pbm"))
    if not pbms:
        raise SystemExit(f"no frames in {frames_dir}")

    styled = [stylise(Image.open(p)) for p in pbms]
    # Hold each frame ~1.3s, the last (sleep face) a touch longer.
    durations = [1300] * len(styled)
    durations[-1] = 2200

    out_path.parent.mkdir(parents=True, exist_ok=True)
    styled[0].save(
        out_path,
        save_all=True,
        append_images=styled[1:],
        duration=durations,
        loop=0,
        optimize=True,
    )
    print("wrote", out_path, f"({len(styled)} frames)")


if __name__ == "__main__":
    main()
