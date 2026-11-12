#!/bin/bash
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
# start DOSBox with WAR.EXE on display :99
pkill -f "Xvfb :99" ; pkill dosbox; sleep 0.5
Xvfb :99 -screen 0 1024x768x24 >/dev/null 2>&1 &
sleep 1
DISPLAY=:99 dosbox -conf $DOS/${CONF:-dosbox.conf} >$DOS/dosbox.log 2>&1 &
sleep 2
