#!/bin/bash
# xshot.sh name: the DOSBox-X picture (640x400 under its menu bar) to $DOS/shots/name.png
W=$(DISPLAY=:99 xdotool search --class dosbox | tail -1)
eval $(DISPLAY=:99 xdotool getwindowgeometry --shell $W)
DISPLAY=:99 import -window root -crop 640x400+$X+$((Y+17)) +repage ${DOS:-/root/dos}/shots/$1.png
