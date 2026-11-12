#!/bin/bash
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
# dostime.sh WORD: press T until the command line shows Time (WORD)
W=$(DISPLAY=:99 xdotool search --class dosbox | head -1)
for i in $(seq 12); do
  bash $TOOLS/dosshot.sh w >/dev/null; python3 $TOOLS/screentext.py $DOS/shots/w.png 2>/dev/null | tail -1 | grep -q "($1)" && exit 0
  DISPLAY=:99 xdotool keydown --window $W t; sleep 0.3; DISPLAY=:99 xdotool keyup --window $W t; sleep 1.2
done; exit 1
