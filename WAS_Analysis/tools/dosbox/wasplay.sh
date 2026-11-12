#!/bin/bash
# start WAS.EXE in DOSBox and go to the first campaign's map
TOOLS=$(cd "$(dirname "$0")" && pwd); WAR=$TOOLS/../../../WAR_Analysis/tools/dosbox
CONF=${CONF:-dosbox_was.conf} bash $WAR/dosrun2.sh
sleep 9
for i in 1 2 3 4 5; do bash $WAR/doskey.sh Return sleep:2.5; done
sleep 2
