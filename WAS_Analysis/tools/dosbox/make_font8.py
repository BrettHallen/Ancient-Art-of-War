#!/usr/bin/env python3
"""make_font8.py [WAS.EXE] > font8.txt: the game's 8x8 font for WAR's
screentext.py (the font is the same as WAR.EXE's; it is not kept in the repo).
One line per character: code, then the 8 row bytes."""
import os
import sys

here = os.path.dirname(os.path.abspath(__file__))
exe = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, '..', '..', '..', 'WAS_Original_Files_3.5', 'WAS.EXE')
d = open(exe, 'rb').read()
ds = 0x23AB0 + 0x089C       # the data segment (file 23AB0h) + the font at DS:089C
for c in range(95):
    print(' '.join([str(32 + c)] + [str(b) for b in d[ds + c * 8:ds + c * 8 + 8]]))
