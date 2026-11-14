#!/bin/bash
# wquote.sh MACHINE: WARET.EXE: the quote and PREPARE FOR WAR! (shots x_MACHINE_qNN every 0.5 s)
m=$1; export HOLD=0.15
CONF=dosboxx_et_$m.conf bash /root/bin/dosrunx.sh
sleep 4; /root/bin/dosanswer.sh >/dev/null || exit 1
sleep 6
for k in Return Return Return Return; do /root/bin/doskey.sh $k; sleep 4; done
/root/bin/doskey.sh Return
for i in $(seq -w 1 16); do sleep 0.5; /root/bin/dosshot.sh x_${m}_q$i; done
