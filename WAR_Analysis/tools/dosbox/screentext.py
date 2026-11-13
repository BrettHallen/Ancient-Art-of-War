#!/usr/bin/env python3
# screentext.py SHOT.png: the text rows of a DOSBox screenshot (640x400 at 192,184 capture, or a 720x348 Hercules screen), read with the game's 8x8 font
import signal; signal.signal(signal.SIGPIPE, signal.SIG_DFL)
import sys, os, subprocess
from PIL import Image
font = {}
for line in open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'font8.txt')):
    v = list(map(int, line.split())); font[tuple(v[1:])] = chr(v[0])
im = Image.open(sys.argv[1]).convert('L')
W, H = im.size
if (W, H) == (720, 348):          # a Hercules screen (WAR HERC): CGA pixel x,y is the pair 32+2x, 72+y
    h = im.load()
    im = Image.new('L', (320, 200))
    im.putdata([max(h[32 + 2 * x, 72 + y], h[33 + 2 * x, 72 + y]) for y in range(200) for x in range(320)])
    W, H = im.size
sx, sy = W // 320, H // 200
px = im.load()
for r in range(25):
    s = ''
    for c in range(40):
        rows = []
        for y in range(8):
            b = 0
            for x in range(8):
                b = b * 2 + (px[(c * 8 + x) * sx, (r * 8 + y) * sy] > 40)
            rows.append(b)
        t = tuple(rows)
        if t in font: s += font[t]
        else:
            inv = tuple(255 - v for v in rows)
            s += font.get(inv, '#')
    print(s.rstrip())
