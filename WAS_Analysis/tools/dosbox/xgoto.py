#!/usr/bin/env python3
"""xgoto.py X CY: as wasgoto.py, for DOSBox-X (xshot.sh / xkey.sh): moves the
marker to byte X, line pair CY by tapping the keypad and reading the screen."""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from findcur import find  # noqa: E402

DOS = os.environ.get('DOS', '/root/dos')
tx, ty = int(sys.argv[1]), int(sys.argv[2])
top = 109
KEYS = {(-1, -1): 'KP_Home', (0, -1): 'KP_Up', (1, -1): 'KP_Prior', (-1, 0): 'KP_Left', (1, 0): 'KP_Right',
        (-1, 1): 'KP_End', (0, 1): 'KP_Down', (1, 1): 'KP_Next'}
for it in range(80):
    subprocess.run([os.path.join(HERE, 'xshot.sh'), 'goto'], check=True)
    c = find(os.path.join(DOS, 'shots', 'goto.png'))
    if not c:
        print('no marker'); break
    cx, cy = c[0][0], top + c[0][1] // 2
    if (cx, cy) == (tx, ty):
        print('at', cx, cy); break
    d = ((tx > cx) - (tx < cx), (ty > cy) - (ty < cy))
    hold = 0.05 if max(abs(tx - cx), abs(ty - cy)) < 4 else 0.3
    subprocess.run([os.path.join(HERE, 'xkey.sh'), KEYS[d]], env=dict(os.environ, HOLD=str(hold)))
else:
    print('gave up')
