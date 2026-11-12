#!/usr/bin/env python3
"""compare_shots.py DOSBOX.png REMAKE.bmp [FIRST-LINE LAST-LINE]: pixel differences
between a DOSBox screenshot of the original (640x400, as WAR_Analysis/tools/dosbox/
dosshot.sh takes them) and a remake screen (320x200, from test_play).

The Ancient Art of War at Sea (1987) - remake
Nov 2026 using Claude Opus 5.5, "Guided" by Brett Hallen
"""
import sys
from PIL import Image

a = Image.open(sys.argv[1]).convert('RGB').resize((320, 200), Image.NEAREST)
b = Image.open(sys.argv[2]).convert('RGB')
y0, y1 = (int(sys.argv[3]), int(sys.argv[4])) if len(sys.argv) > 4 else (0, 200)
diff = [(x, y) for y in range(y0, y1) for x in range(320) if a.getpixel((x, y)) != b.getpixel((x, y))]
print(f'{len(diff)} pixels differ' + (f'; first {diff[:8]}' if diff else ''))
sys.exit(1 if diff else 0)
