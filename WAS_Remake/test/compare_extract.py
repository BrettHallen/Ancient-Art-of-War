#!/usr/bin/env python3
"""Compare the remake's pictures and tiles with WAS_Graphics/extract_graphics.py.

The Ancient Art of War at Sea (1987) - remake
Nov 2026 using Claude Opus 5.5, "Guided" by Brett Hallen

Two decoders written separately (C in src/assets.c, Python in
WAS_Graphics/extract_graphics.py) must agree pixel for pixel.

Usage: compare_extract.py SHOTS-FOLDER   (test_viewer's BMPs)
Needs Pillow; reads the game files from ../../WAS_Original_Files_3.5 through
the extractor's own functions.
"""
import os
import sys
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'WAS_Graphics'))
import extract_graphics as E  # noqa: E402

ORDER = ['SPM', 'SPI', 'SPE', 'SPF', 'SPJ', 'SPG', 'SPH', 'SPK', 'SPN', 'SPO', 'SPP', 'SPQ', 'AD', 'B']
REC16 = {'SPM': 1, 'SPI': 2, 'SPE': 3, 'SPF': 4, 'SPJ': 5, 'SPG': 6, 'SPH': 7, 'SPK': 8,
         'SPN': 10, 'SPO': 9, 'AD': 13, 'B': 29}


def same(a, b):
    a, b = a.convert('RGB'), b.convert('RGB')
    if a.size != b.size:
        return False
    return a.tobytes() == b.tobytes()


def main():
    shots = sys.argv[1]
    bad = 0
    recs = E.wast_records()
    for i, name in enumerate(ORDER):
        cga = Image.open(os.path.join(shots, f'CGA_{i + 1:02d}.bmp'))
        if not same(cga, E.cga_picture(name)):
            print('CGA differs:', name); bad += 1
        if name in REC16:
            t = Image.open(os.path.join(shots, f'Tandy_{i + 1:02d}.bmp'))
            if not same(t, E.vga_picture(recs[REC16[name]])):
                print('16 colours differ:', name); bad += 1
    # the tile screen: 10 across, so 94 tiles; compare each 32x20 cell
    tiles = {'CGA': E.cga_tiles(), 'Tandy': []}
    for r in E.VGA_TILE_RECORDS:
        tiles['Tandy'] += E.vga_tiles(recs[r])
    for mode, tl in tiles.items():
        sheet = Image.open(os.path.join(shots, f'{mode}_{len(ORDER) + 1:02d}.bmp')).convert('RGB')
        for k, t in enumerate(tl):
            x, y = (k % 10) * 32, (k // 10) * 20
            if not same(sheet.crop((x, y, x + 32, y + 20)), t):
                print(mode, 'tile differs:', k + 1); bad += 1
    print('compare_extract:', 'passed' if not bad else f'{bad} differences')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
