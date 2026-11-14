#!/bin/bash
# wsaveset.sh start|play|mark: saved games for WAR_Remake/test/test_set, copied to $DOS/sets/ (DOS default /root/dos)
#   start: the first campaign saved as it reaches the map, by WARET.EXE (SET_start) and by WAR.EXE (S_start)
#   play:  WARET.EXE: squad 16 traced north-west, time fast, saved 25 s later (SET_play)
#   mark:  WARET.EXE: squad 16 on a long traced path, saved at once, its mark showing (SET_mark)
DOS=${DOS:-/root/dos}; mkdir -p $DOS/sets; export HOLD=0.15
save(){   # Alt+S, then the disk prompt; the file goes to the data disk on B: ($DOS/data_bx)
  /root/bin/doskey.sh alt+s; sleep 2; /root/bin/doskey.sh Return; sleep 3
  pkill -9 -f dosbox-x
}
tomap(){  # CONF: through the menus to the first campaign's map
  CONF=$1 bash /root/bin/dosrunx.sh
  sleep 4; /root/bin/dosanswer.sh >/dev/null || exit 1
  sleep 6
  for k in Return Return Return Return Return; do /root/bin/doskey.sh $k; sleep 4; done
  /root/bin/doswait.sh "Surrender" 30 || exit 1
}
k(){ for i in $(seq $2); do HOLD=0 /root/bin/doskey.sh $1; sleep 0.3; done; }
case $1 in
start)
  rm -f $DOS/data_bx/SET $DOS/data_bx/S
  tomap dosboxx_et_ega.conf; save; cp $DOS/data_bx/SET $DOS/sets/SET_start
  tomap dosboxx_et_cga.conf; save; cp $DOS/data_bx/S $DOS/sets/S_start ;;
play|mark)
  rm -f $DOS/data_bx/SET
  bash /root/bin/wtosquad.sh ega || exit 1
  HOLD=0 /root/bin/doskey.sh m; sleep 1
  if [ $1 = play ]; then
    k Up 8; k Left 4; HOLD=0 /root/bin/doskey.sh m; sleep 1
    for i in 1 2 3; do /root/bin/doskey.sh t; sleep 1; done
    sleep 25
  else
    k Up 14; k Right 6; HOLD=0 /root/bin/doskey.sh m; sleep 2
  fi
  /root/bin/dosshot.sh x_set_$1
  save; cp $DOS/data_bx/SET $DOS/sets/SET_$1 ;;
*) echo "wsaveset.sh start|play|mark"; exit 2 ;;
esac
