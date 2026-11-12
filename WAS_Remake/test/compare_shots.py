#!/usr/bin/env python3
"""compare_shots.py DOSBOX.png REMAKE.bmp [FIRST-LINE LAST-LINE]: pixel differences
between a DOSBox screenshot of the original (640x400, as WAR_Analysis/tools/dosbox/
dosshot.sh takes them) and a remake screen (320x200, from test_play). A Hercules
screenshot (720x348, machine=hercules) is compared with a 720x348 remake screen as it is.

The Ancient Art of War at Sea (1987) - remake
Nov 2026 using Claude Opus 5.5, "Guided" by Brett Hallen
"""
import sys
from PIL import Image

a = Image.open(sys.argv[1]).convert('RGB')
b = Image.open(sys.argv[2]).convert('RGB')
if a.size != b.size:
    a = a.resize(b.size, Image.NEAREST)
w, h = b.size
y0, y1 = (int(sys.argv[3]), int(sys.argv[4])) if len(sys.argv) > 4 else (0, h)
diff = [(x, y) for y in range(y0, y1) for x in range(w) if a.getpixel((x, y)) != b.getpixel((x, y))]
print(f'{len(diff)} pixels differ' + (f'; first {diff[:8]}' if diff else ''))
sys.exit(1 if diff else 0)
