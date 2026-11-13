#!/bin/bash
# wmenus.sh MACHINE: WARET.EXE in DOSBox-X: title page -> campaign menu (each item) -> Options Shoppe;
# screenshots x_MACHINE_mNN
m=$1; export HOLD=0.15
CONF=dosboxx_et_$m.conf bash /root/bin/dosrunx.sh
sleep 4; /root/bin/dosanswer.sh >/dev/null
sleep 8
/root/bin/doskey.sh equal; sleep 1; /root/bin/dosshot.sh x_${m}_m01   # title, make changes
/root/bin/doskey.sh Return; sleep 4; /root/bin/dosshot.sh x_${m}_m02  # campaign menu
for k in 3 4 5 6 7; do /root/bin/doskey.sh equal; sleep 1.5; /root/bin/dosshot.sh x_${m}_m0$k; done
/root/bin/doskey.sh Return; sleep 4; /root/bin/dosshot.sh x_${m}_m08  # Options Shoppe?
