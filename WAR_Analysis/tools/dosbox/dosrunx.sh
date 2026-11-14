#!/bin/bash
# dosrunx.sh: start DOSBox-X with /root/dos/$CONF on display :99 (its log goes nowhere: it grows by gigabytes)
pkill -f "Xvfb :99" ; pkill dosbox; pkill -9 -f dosbox-x; sleep 0.5   # DOSBox-X ignores TERM
Xvfb :99 -screen 0 1024x768x24 >/dev/null 2>&1 &
sleep 1
DISPLAY=:99 dosbox-x -conf /root/dos/${CONF} >/dev/null 2>&1 &
sleep 3
