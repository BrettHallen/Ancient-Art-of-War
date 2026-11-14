#!/bin/bash
# wtosquad.sh MACHINE: WARET.EXE to the map (wtomap.sh), then the cursor onto your squad 16 (screen x 164, line 164)
m=$1
bash /root/bin/wtomap.sh $m >/dev/null 2>&1 || exit 1
python3 /root/bin/steer.py 164 164
