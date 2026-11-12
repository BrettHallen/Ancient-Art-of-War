/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fort_trace.c: a test or debugging aid for the remake (see README.md here)
 */
/* A fort battle, arrow by arrow: where each of my arrows ends, and where the enemy stands */
static int force_bg = 3;
#define BATTLE_FORCE_BG force_bg
#include "../src/play.c"
#include "../src/battle.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play play; static Battle bt;
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) return 1;
    campaigns_load(camps, argv[1]);
    int run = atoi(argv[2]); if (run < 0) run = 0, run = -1;
    play_start(&play, &camps[0], &ga, 100 + (run < 0 ? 0 : run));
    PSquad *a = &play.sq[16], *e = &play.sq[6];
    a->archers = 10; a->knights = 0; a->barbarians = 0; a->spies = 0;
    e->archers = 0; e->knights = 7; e->barbarians = 7;
    squads_update(&play);
    a->x = e->x + 1; a->y = e->y + 3; play.time_div = 1;
    for (int f = 0; f < 50 && !a->fighting; f++) play_frame(&play, 0);
    battle_start(&bt, &play, 16);
    if (run < 0) {
        for (int d = 0; d <= 20; d++)
            SDL_Log("dir %2d: w %d h %d dx %d dy %d addr %d", d, rdb(&bt, O7(0x1F6) + d), rdb(&bt, O7(0x20B) + d), rdb(&bt, O7(0x220) + d), rdb(&bt, O7(0x235) + d), rd(&bt, O7(0x24A) + 2 * d));
        return 0;
    }
    int px[MAX_ARROWS], py[MAX_ARROWS];
    for (int k = 0; k < MAX_ARROWS; k++) px[k] = bt.ax[k], py[k] = bt.ay[k];
    for (int f = 0; f < 400 && !bt.done; f++) {
        if (f == 2) battle_key(&bt, PK_A);
        battle_frame(&bt);
        if (f % 5 == 0) {
            char line[400]; int n = SDL_snprintf(line, sizeof line, "f%3d r%d en:", f, bt.retreat);
            for (int j = 0; j < MAX_MEN; j++) if (bt.addr[j] != -1 && bt.side[j] == ENEMY && bt.stat[j] != DEAD) n += SDL_snprintf(line + n, sizeof line - n, " %d,%d", bt.x[j], bt.y[j]);
            SDL_Log("%s", line);
        }
        for (int k = 0; k < MAX_ARROWS; k++) {
            if (bt.aside[k] == FRIEND && px[k] != 0 && px[k] != 9999 && bt.ax[k] != 0 && bt.ax[k] != 9999 && px[k] == bt.ax[k] - rdb(&bt, O7(0x220) + bt.akind[k]))
                ;
            if (bt.aside[k] == FRIEND && (px[k] == 9999) && bt.ax[k] != 9999 && bt.ax[k] != 0)
                SDL_Log("  f%3d arrow %d loosed dir %d at %d,%d pow %d", f, k, bt.akind[k], bt.ax[k], bt.ay[k], bt.apow[k]);
            if (bt.aside[k] == FRIEND && px[k] != 0 && px[k] != 9999 && bt.ax[k] == 0)
                SDL_Log("  f%3d arrow %d gone after %d,%d (dir %d pow %d)", f, k, px[k], py[k], bt.akind[k], bt.apow[k]);
            px[k] = bt.ax[k]; py[k] = bt.ay[k];
        }
    }
    SDL_Log("lost en %d", bt.lost_en);
    return 0;
}
