/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/t_mov.c: a test or debugging aid for the remake (see README.md here)
 */
#include "../src/play.c"
#include <stdio.h>
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play play;
int main(int argc, char **argv){ (void)argc; char e[256]; assets_load(&ga, argv[1], e, 256); campaigns_load(camps, argv[1]);
 for (int c=0;c<NUM_CAMPAIGNS;c++){ if(!camps[c].loaded) continue; const uint8_t *a=camps[c].armies; printf("c%d:",c);
  for(int i=0;i<20;i++){int mv=(int16_t)(a[80+2*i]|a[81+2*i]<<8), sp=(int16_t)(a[40+2*i]|a[41+2*i]<<8), side=(int16_t)(a[2*i]|a[2*i+1]<<8); if(mv!=9999) printf(" %s%d/%d", side==-1?"Y":"E", mv, sp);} printf("\n");}}
