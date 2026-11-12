#!/bin/bash
# build.sh NAME [compiler flags]: build test/NAME.c against the remake's sources, with ASan and UBSan.
# The sources a test #includes (menu.c, play.c, battle.c, main.c ...) are left out of the link;
# main.c always is. SDL3 comes from pkg-config, or from SDL3_DIR (include/ and lib/).
cd "$(dirname "$0")"
n=${1%.c}; shift
inc=$(grep -o '#include "../src/[a-z_0-9]*\.c"' "$n.c" | sed 's/.*src\///; s/"//')
srcs=""
for f in ../src/*.c; do
  b=$(basename "$f")
  [ "$b" = main.c ] && continue
  echo "$inc" | grep -qx "$b" && continue
  srcs="$srcs $f"
done
if [ -n "$SDL3_DIR" ]; then
  SDL_CFLAGS="-I$SDL3_DIR/include"; SDL_LIBS="-L$SDL3_DIR/lib -lSDL3 -Wl,-rpath,$SDL3_DIR/lib"
else
  SDL_CFLAGS=$(pkg-config --cflags sdl3); SDL_LIBS=$(pkg-config --libs sdl3)
fi
${CC:-cc} -g -O1 -fsanitize=address,undefined -I../src $SDL_CFLAGS "$@" "$n.c" $srcs $SDL_LIBS -lm -o "$n"
