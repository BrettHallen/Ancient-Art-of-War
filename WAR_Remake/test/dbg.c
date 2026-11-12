/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/dbg.c: a test or debugging aid for the remake (see README.md here)
 */
#define BATTLE_TRACE(...) SDL_Log(__VA_ARGS__)
#include "../src/play.c"
#include "../src/battle.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play play; static Battle bt;
int main(int argc, char **argv){ (void)argc; char err[256];
 assets_load(&ga, argv[1], err, sizeof err); campaigns_load(camps, argv[1]);
 play_start(&play, &camps[0], &ga, 7);
 PSquad *a = &play.sq[18], *e = &play.sq[6]; a->x = e->x + 1; a->y = e->y + 3; play.time_div = 1;
 for (int f = 0; f < 50 && !a->fighting; f++) play_frame(&play, 0);
 battle_start(&bt, &play, 18);
 for (int i=0;i<MAX_MEN;i++) if(bt.addr[i]!=-1) SDL_Log("man %d side %d type %d x %d y %d", i, bt.side[i], bt.type[i], bt.x[i], bt.y[i]);
 for (int f=0; f<40; f++){ if(f==25) battle_key(&bt, PK_A); battle_frame(&bt);
   for(int k=0;k<MAX_ARROWS;k++) if(bt.ax[k]) SDL_Log("f%d arrow %d ax %d ay %d dir %d pow %d", f,k,bt.ax[k],bt.ay[k],bt.akind[k],bt.apow[k]);
   if (f%5==0) for (int i=0;i<MAX_MEN;i++) if(bt.addr[i]!=-1 && bt.side[i]==0) SDL_Log("  f%d en %d x %d y %d stat %d", f,i,bt.x[i],bt.y[i],bt.stat[i]); }
}
