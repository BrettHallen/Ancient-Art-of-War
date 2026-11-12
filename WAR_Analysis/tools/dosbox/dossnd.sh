#!/bin/bash
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
# start DOSBox with sound (captured with Ctrl+F6 into $DOS/cap), answer the manual question
pkill -f "Xvfb :99" ; pkill dosbox; sleep 0.5
mkdir -p $DOS/cap
Xvfb :99 -screen 0 1024x768x24 >/dev/null 2>&1 &
sleep 1
DISPLAY=:99 SDL_AUDIODRIVER=${AUDIO:-dummy} dosbox -conf ${CONF:-$DOS/dosbox_snd.conf} >$DOS/dosbox.log 2>&1 &
sleep 4
