#!/bin/bash
# wcgapat.sh: CGAPAT.COM (cgapat.py, in $DOS/comptest) on the old and the new CGA's composite output in DOSBox-X:
# shots pat_MACHINE_p1 (palette 1) and pat_MACHINE_p0 (palette 0), for complut.py
for m in cga_composite cga_composite2; do
  CONF=dosboxx_pat_$m.conf bash /root/bin/dosrunx.sh
  sleep 5; /root/bin/dosshot.sh pat_${m}_p1
  /root/bin/doskey.sh space; sleep 2; /root/bin/dosshot.sh pat_${m}_p0
done
pkill -9 -f dosbox-x
