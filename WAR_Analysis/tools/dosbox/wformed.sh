#!/bin/bash
# wformed.sh MACHINE: WARET.EXE: Teach squads new formations: formations 1, 3 and 6, the pointer; shots x_MACHINE_fNN
m=$1; export HOLD=0.15
CONF=dosboxx_et_$m.conf bash /root/bin/dosrunx.sh
sleep 4; /root/bin/dosanswer.sh >/dev/null || exit 1
sleep 8
s(){ sleep ${2:-2}; /root/bin/dosshot.sh x_${m}_f$1; }
/root/bin/doskey.sh equal; sleep 1; /root/bin/doskey.sh Return; sleep 4
for i in 1 2 3 4; do /root/bin/doskey.sh equal; sleep 1.5; done; s 01      # Teach squads new formations
/root/bin/doskey.sh Return; s 02 4                                        # formation 1 (or the data disk)
/root/bin/doskey.sh equal; s 03                                           # formation 2
for i in 1 2 3 4 5 6; do                                                  # on to formation 6
  python3 /root/bin/screentext.py /root/dos/shots/x_${m}_f04.png 2>/dev/null | grep -q "Formation: 6" && break
  HOLD=0.05 /root/bin/doskey.sh equal; s 04 1
done
HOLD=0.05 /root/bin/doskey.sh m; s 05                                      # the pointer
HOLD=0.05 /root/bin/doskey.sh Left; sleep 0.5; HOLD=0.05 /root/bin/doskey.sh Left; s 06   # moved
