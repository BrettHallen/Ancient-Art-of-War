#!/bin/bash
# wloadset.sh MACHINE: WARET.EXE: load SET from the data disk (B:), then a shot every 4 s for a minute (x_MACHINE_lNN)
m=$1; export HOLD=0.15
CONF=dosboxx_et_$m.conf bash /root/bin/dosrunx.sh
sleep 4; /root/bin/dosanswer.sh >/dev/null || exit 1
sleep 6; /root/bin/doskey.sh Return
/root/bin/doswait.sh "Select a campaign" 30 || exit 1
for i in $(seq 13); do /root/bin/doskey.sh equal; done
/root/bin/doskey.sh Return
/root/bin/doswait.sh "press any key" 10 && /root/bin/doskey.sh Return
/root/bin/doswait.sh "Saved game" 20 || exit 1
/root/bin/doskey.sh s
for i in $(seq -w 1 15); do sleep 4; /root/bin/dosshot.sh x_${m}_l$i; done
