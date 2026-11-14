#!/bin/bash
# xkey.sh key... : keys to the DOSBox-X window (sleep:N waits)
W=$(DISPLAY=:99 xdotool search --class dosbox | tail -1)
for k in "$@"; do
  if [[ $k == sleep:* ]]; then sleep ${k#sleep:}; continue; fi
  DISPLAY=:99 xdotool keydown --window $W "$k"; sleep ${HOLD:-0.12}; DISPLAY=:99 xdotool keyup --window $W "$k"; sleep 0.3
done
