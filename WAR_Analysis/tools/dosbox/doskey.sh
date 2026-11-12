#!/bin/bash
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
# doskey.sh key... : send keys to DOSBox (xdotool names, a+b for chords, sleep:N to wait)
W=$(DISPLAY=:99 xdotool search --class dosbox | head -1)
for k in "$@"; do
  if [[ $k == sleep:* ]]; then sleep ${k#sleep:}; continue; fi
  IFS='+' read -ra parts <<< "$k"
  for p in "${parts[@]}"; do DISPLAY=:99 xdotool keydown --window $W "$p"; sleep 0.08; done
  sleep ${HOLD:-0.12}
  for ((i=${#parts[@]}-1; i>=0; i--)); do DISPLAY=:99 xdotool keyup --window $W "${parts[i]}"; sleep 0.05; done
  sleep 0.3
done
