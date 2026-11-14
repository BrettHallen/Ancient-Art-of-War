#!/bin/bash
# wedit.sh MACHINE: WARET.EXE: Make a new campaign: the map maker and the squads (Size, Info, Formation); shots
# x_MACHINE_eNN (WAR_Remake/test/dump_editors sends the same keys; DOSBox reads some held keys twice: see eNN there)
m=$1; export HOLD=0.15
CONF=dosboxx_et_$m.conf bash /root/bin/dosrunx.sh
sleep 4; /root/bin/dosanswer.sh >/dev/null || exit 1
sleep 8
s(){ sleep ${2:-2}; /root/bin/dosshot.sh x_${m}_e$1; }
/root/bin/doskey.sh equal; sleep 1; /root/bin/doskey.sh Return; s 01 4     # campaign menu
/root/bin/doskey.sh Return; s 02                                          # put the data disk in B
/root/bin/doskey.sh Return; s 03 6                                        # map maker intro
/root/bin/doskey.sh Return; s 04 3                                        # map
/root/bin/doskey.sh Right; /root/bin/doskey.sh Down; s 14                 # the cursor one cell right and down
/root/bin/doskey.sh Return; s 05 3                                        # continue?
/root/bin/doskey.sh y; s 06                                               # squads intro
/root/bin/doskey.sh space; s 07                                           # squads main
/root/bin/doskey.sh s; s 08                                               # size
for i in 1 2 3; do /root/bin/doskey.sh k; sleep 0.4; done; s 09           # size filled
/root/bin/doskey.sh e; s 10                                               # squad menu
/root/bin/doskey.sh i; s 11                                               # info
/root/bin/doskey.sh e; s 12
/root/bin/doskey.sh f; s 13                                               # formation
