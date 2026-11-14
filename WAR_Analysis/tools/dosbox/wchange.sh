#!/bin/bash
# wchange.sh MACHINE: WARET.EXE: Change a campaign: the scroll of campaigns, the editor's menu; shots x_MACHINE_cNN
m=$1; export HOLD=0.15
CONF=dosboxx_et_$m.conf bash /root/bin/dosrunx.sh
sleep 4; /root/bin/dosanswer.sh >/dev/null || exit 1
sleep 8
s(){ sleep ${2:-2}; /root/bin/dosshot.sh x_${m}_c$1; }
/root/bin/doskey.sh equal; sleep 1; /root/bin/doskey.sh Return; sleep 4
/root/bin/doskey.sh equal; s 01                                           # Change a campaign
/root/bin/doskey.sh Return; s 02 4                                        # the data disk / the campaigns
/root/bin/doskey.sh Return; s 03 4
/root/bin/doskey.sh Return; s 04 4
/root/bin/doskey.sh equal; s 05
/root/bin/doskey.sh equal; s 06
