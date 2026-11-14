#!/bin/bash
# start DOSBox-X on a fresh :99 with CONF
pkill -9 -f "Xvfb :99"; pkill -9 -f dosbox-x; sleep 0.5
rm -f /tmp/.X99-lock /tmp/.X11-unix/X99    # left behind by kill -9: a new Xvfb :99 would not start
Xvfb :99 -screen 0 1024x768x24 >/dev/null 2>&1 &
sleep 1
DISPLAY=:99 dosbox-x -conf ${CONF:-${DOS:-/root/dos}/dosboxx_wastu.conf} >/dev/null 2>&1 &
sleep ${WAIT:-9}
