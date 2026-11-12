#!/bin/bash
TOOLS=$(cd "$(dirname "$0")" && pwd); DOS=${DOS:-/root/dos}
# one fort battle in the original from data_b/S: load, Zoom, record it; prints "They lost N men"
bash $TOOLS/dosloads.sh >/dev/null || { echo load-failed; exit 1; }
sleep 1; HOLD=0.15 bash $TOOLS/doskey.sh z
D=$DOS/fortcase/run_$1; rm -rf $D; mkdir -p $D
timeout 20 ffmpeg -loglevel error -f x11grab -framerate 10 -video_size 640x400 -i :99.0+192,184 -t 15 $D/v.mp4
ffmpeg -loglevel error -i $D/v.mp4 -vf fps=10 $D/%03d.png
for f in $D/[0-9]*.png; do
  T=$(python3 $TOOLS/screentext.py $f | tail -2 | tr '\n' ' ')
  case "$T" in *"They lost"*) echo "$(basename $f) $T"; exit 0;; esac
done
echo "no result"
