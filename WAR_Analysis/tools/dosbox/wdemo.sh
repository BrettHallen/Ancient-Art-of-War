#!/bin/bash
# wdemo.sh MACHINE: WARET.EXE in DOSBox-X: wait on the title page for the demo and record 130 s at 2 frames a
# second into /root/dos/xdemo_MACHINE/ (the demo games end on the battle screen)
m=$1; D=/root/dos/xdemo_$m; rm -rf $D; mkdir -p $D
CONF=dosboxx_et_$m.conf bash /root/bin/dosrunx.sh
sleep 4; /root/bin/dosanswer.sh >/dev/null || exit 1
W=$(DISPLAY=:99 xdotool search --class dosbox | head -1)
eval $(DISPLAY=:99 xdotool getwindowgeometry --shell $W)
timeout 140 ffmpeg -loglevel error -f x11grab -framerate 2 -video_size ${WIDTH}x${HEIGHT} -i :99.0+$X,$Y -t 130 -c:v png $D/%03d.png
