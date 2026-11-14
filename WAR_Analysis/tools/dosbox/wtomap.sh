#!/bin/bash
# wtomap.sh MACHINE: WARET.EXE in DOSBox-X to the map of the first campaign
m=$1; export HOLD=0.15
CONF=dosboxx_et_$m.conf bash /root/bin/dosrunx.sh
sleep 4; /root/bin/dosanswer.sh >/dev/null || exit 1
sleep 6
for k in Return Return Return Return Return; do /root/bin/doskey.sh $k; sleep 4; done
/root/bin/doswait.sh "Surrender" 30
