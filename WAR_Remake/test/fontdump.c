/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fontdump.c: a test or debugging aid for the remake (see README.md here)
 */
#include "../src/assets.h"
#include "../src/gfx.h"
#include <stdio.h>
static GameAssets ga; static Screen s;
int main(int c, char **v){ char e[256]; (void)c; assets_load(&ga, v[1], e, 256);
  screen_set_palette(&s, CGA_PALETTE, 4);
  for (int ch = 32; ch < 127; ch++) { char t[2] = { (char)ch, 0 }; screen_fill(&s, 0, 0, 8, 8, 0); screen_text(&s, &ga, 0, 0, t, 3, 0);
    printf("%d", ch); for (int y = 0; y < 8; y++) { int b = 0; for (int x = 0; x < 8; x++) b = b * 2 + (s.px[y * SCREEN_W + x] != 0); printf(" %d", b); } printf("\n"); }
  return 0; }
