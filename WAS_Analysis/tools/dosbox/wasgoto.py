#!/usr/bin/env python3
"""wasgoto.py X CY [VIEWTOP]: move WAS.EXE's marker in DOSBox to byte X,
line pair CY (the view's top line pair, default 109 = the bottom of the map,
must not change on the way). Taps the keypad and reads the screen."""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
WAR = os.path.join(HERE, '..', '..', '..', 'WAR_Analysis', 'tools', 'dosbox')
DOS = os.environ.get('DOS', '/root/dos')
sys.path.insert(0, HERE)
from findcur import find  # noqa: E402

tx, ty = int(sys.argv[1]), int(sys.argv[2])
top = int(sys.argv[3]) if len(sys.argv) > 3 else 109
W = subprocess.run('DISPLAY=:99 xdotool search --class dosbox | head -1', shell=True,
                   capture_output=True, text=True).stdout.strip()
KEYS = {(-1, -1): 'KP_Home', (0, -1): 'KP_Up', (1, -1): 'KP_Prior', (-1, 0): 'KP_Left', (1, 0): 'KP_Right',
        (-1, 1): 'KP_End', (0, 1): 'KP_Down', (1, 1): 'KP_Next'}
for it in range(80):
    subprocess.run(['bash', os.path.join(WAR, 'dosshot.sh'), 'goto'], check=True)
    c = find(os.path.join(DOS, 'shots', 'goto.png'))
    if not c:
        print('no marker'); break
    cx, cy = c[0][0], top + c[0][1] // 2
    if (cx, cy) == (tx, ty):
        print('at', cx, cy); break
    d = ((tx > cx) - (tx < cx), (ty > cy) - (ty < cy))
    hold = 0.05 if max(abs(tx - cx), abs(ty - cy)) < 4 else 0.3
    subprocess.run(f'DISPLAY=:99 xdotool keydown --window {W} {KEYS[d]}; sleep {hold}; '
                   f'DISPLAY=:99 xdotool keyup --window {W} {KEYS[d]}; sleep 0.3', shell=True)
else:
    print('gave up')
