#!/bin/bash
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
# dosshot.sh name : screenshot the DOSBox window to $DOS/shots/name.png
mkdir -p $DOS/shots
W=$(DISPLAY=:99 xdotool search --class dosbox | head -1)
DISPLAY=:99 import -window "$W" $DOS/shots/$1.png
