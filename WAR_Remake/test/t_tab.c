/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/t_tab.c: a test or debugging aid for the remake (see README.md here)
 */
#include "../src/assets.h"
#include <stdio.h>
static GameAssets ga;
int main(int c, char**v){ char e[256]; if(!assets_load(&ga,v[1],e,256)) return 1;
 const char *n[5]={"w","h","dx","dy","dad"}; int o[5]={0x1F6,0x20B,0x220,0x235,0x24A};
 for(int t=0;t<5;t++){printf("%-4s",n[t]); for(int k=0;k<21;k++) printf("%5d",(signed char)ga.o7[o[t]+k]); printf("\n");}
 printf("AB99:"); for(int k=0;k<14;k++) printf(" %d",(short)(ga.o7[0x7D0+2*k]|ga.o7[0x7D1+2*k]<<8)); printf("\n");}
