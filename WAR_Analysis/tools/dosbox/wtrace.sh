#!/bin/bash
# wtrace.sh MACHINE: WARET.EXE: onto your squad 16, M, six Ups, four Lefts, M, three Rights: the trace box and the
# path's mark; shots x_MACHINE_uNN. M with HOLD=0: a held M is read twice and ends the trace at once.
m=$1; export HOLD=0.15
bash /root/bin/wtosquad.sh $m || exit 1
s(){ sleep ${2:-1.5}; /root/bin/dosshot.sh x_${m}_u$1; }
k(){ for i in $(seq $2); do HOLD=0 /root/bin/doskey.sh $1; sleep 0.3; done; }
HOLD=0 /root/bin/doskey.sh m; s 01
k Up 6; s 02
k Left 4; s 03
HOLD=0 /root/bin/doskey.sh m; s 04 0.5
k Right 3; s 05 0.5
s 06 2; s 07 3
