#!/bin/bash
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
# capture DOSBox every 0.5 s for N seconds into $DOS/demo
W=$(DISPLAY=:99 xdotool search --class dosbox | head -1)
start=$(date +%s.%N)
end=$(echo "$start + $1" | bc)
i=0
while (( $(echo "$(date +%s.%N) < $end" | bc) )); do
  t=$(echo "$(date +%s.%N) - $start" | bc)
  DISPLAY=:99 import -window "$W" $DOS/demo/$(printf %05d $i)_$(printf %.1f $t).png
  i=$((i+1)); sleep 0.4
done
