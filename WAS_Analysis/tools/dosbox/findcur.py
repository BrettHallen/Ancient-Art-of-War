#!/usr/bin/env python3
"""findcur.py SHOT.png: where WAS.EXE's marker (the cross) is on a DOSBox
screenshot or video frame: [(byte column, screen line)]. Add the view's top
line (2 x DS:3F3B) to the line for the map line; the marker's line pair is
(top line + screen line) / 2. Tolerant of video compression."""
import sys
from PIL import Image


def find(path):
    o = Image.open(path).convert('L').resize((320, 200), Image.NEAREST)
    px = o.load()
    w = lambda x, y: px[x, y] > 200
    b = lambda x, y: px[x, y] < 60
    res = []
    for y in range(0, 172):
        for xb in range(1, 79):
            x = xb * 4
            if all(b(x, y + l) and b(x + 1, y + l) and w(x + 2, y + l) and w(x + 3, y + l) for l in (0, 1, 2, 7, 8, 9)) \
                    and w(x - 2, y + 4) and w(x + 7, y + 4) and b(x - 4, y + 4) and b(x + 4, y + 4):
                res.append((xb, y))
    return res


if __name__ == '__main__':
    print(find(sys.argv[1]))
