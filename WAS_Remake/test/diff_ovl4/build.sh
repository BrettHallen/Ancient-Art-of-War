#!/bin/bash
# build libwas.so (the remake's campaign play, for diff.py and calib.py)
# usage: SDL3_DIR=/path/to/sdl3 ./build.sh
cd "$(dirname "$0")"
S=../../src
SDL=${SDL3_DIR:+-I$SDL3_DIR/include -L$SDL3_DIR/lib -Wl,-rpath,$SDL3_DIR/lib}
cc -O1 -g -shared -fPIC -o libwas.so shim.c $S/play.c $S/enemy.c $S/dsmap.c $S/assets.c $S/campaign.c $S/gfx.c \
   -I$S $SDL -lSDL3
