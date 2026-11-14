#!/bin/bash
# wboxes.sh MACHINE: WARET.EXE: the cursor onto your squad 16, then Info, Size, Formation, Detach, a traced path,
# View, Time, Surrender and the battle report; shots x_MACHINE_kNN (WAR_Remake/test/dump_cards sends the same keys)
m=$1; export HOLD=0.15
bash /root/bin/wtosquad.sh $m || exit 1
s(){ sleep ${2:-1.5}; /root/bin/dosshot.sh x_${m}_k$1; }
s 01
/root/bin/doskey.sh i; s 02; /root/bin/doskey.sh e; sleep 1
/root/bin/doskey.sh s; s 03; /root/bin/doskey.sh e; sleep 1
/root/bin/doskey.sh f; s 04; /root/bin/doskey.sh e; sleep 1
/root/bin/doskey.sh d; s 05; /root/bin/doskey.sh e; sleep 1
python3 /root/bin/steer.py 164 164 >/dev/null
/root/bin/doskey.sh m; s 06
HOLD=0 /root/bin/doskey.sh Up; HOLD=0 /root/bin/doskey.sh Up; s 07
/root/bin/doskey.sh m; s 08
/root/bin/doskey.sh v; s 09 2.5; /root/bin/doskey.sh e; sleep 1
/root/bin/doskey.sh t; s 10
HOLD=0 /root/bin/doskey.sh Up; HOLD=0 /root/bin/doskey.sh Up; HOLD=0 /root/bin/doskey.sh Up; s 11
/root/bin/doskey.sh s; s 12
/root/bin/doskey.sh y; s 13 3
/root/bin/doskey.sh Return; s 14 4
s 15 6
