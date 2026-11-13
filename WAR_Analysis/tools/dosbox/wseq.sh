#!/bin/bash
# wseq.sh MACHINE: WARET.EXE in DOSBox-X (ega, tandy or vgaonly): the opening, the title page, the titles,
# story and rules scrolls, the opponents and the map, a screenshot at each step (x_MACHINE_sNN)
m=$1
CONF=dosboxx_et_$m.conf bash /root/bin/dosrunx.sh
sleep 4
/root/bin/dosanswer.sh
sleep 1; /root/bin/dosshot.sh x_${m}_s01        # presented by
sleep 6; /root/bin/dosshot.sh x_${m}_s02        # title, barbarian
n=3
for k in Return Return Return Return Return Return Return Return; do
  /root/bin/doskey.sh $k; sleep 4
  /root/bin/dosshot.sh x_${m}_s$(printf %02d $n); n=$((n+1))
done
