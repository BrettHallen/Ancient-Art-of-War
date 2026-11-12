/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fort_arrows.c: a test or debugging aid for the remake (see README.md here)
 */
/* My archers in a fort (bg 3) against knights and barbarians: where do their arrows go? */
static int force_bg = -1;
#define BATTLE_FORCE_BG force_bg
static long n_arrow, n_hit_en, n_hit_me, n_kill, n_arrow_r, n_hit_r; static int *cur_retreat;
#define BATTLE_TRACE(fmt, k, side, ...) (SDL_strncmp(fmt, "arrow", 5) == 0 ? ((side) == -1 ? (n_hit_en++, n_hit_r += *cur_retreat != 0) : n_hit_me++) : 0)
#include "../src/play.c"
#include "../src/sound.h"
#define snd_arrow() (n_arrow++, n_arrow_r += *cur_retreat != 0, snd_arrow())
#include "../src/battle.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play play; static Battle bt; static Screen scr;
static uint32_t rgb[SCREEN_W * SCREEN_H];
static void shot(const char *dir, const char *name)
{
    battle_draw(&bt, &scr, MODE_CGA); screen_to_xrgb(&scr, rgb);
    SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char p[512]; SDL_snprintf(p, sizeof p, "%s/%s.bmp", dir, name); SDL_SaveBMP(s, p); SDL_DestroySurface(s);
}
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    if (!assets_load(&ga, argv[2], err, sizeof err)) return 1;
    campaigns_load(camps, argv[2]);
    int bgs[2] = { 3, 12 };
    for (int g = 0; g < 2; g++) {
      long loosed = 0, wall = 0, edge = 0, hits_en = 0, battles = 0, enemy_lost = 0, frames = 0;
      for (int run = 0; run < 40; run++) {
        force_bg = bgs[g];
        play_start(&play, &camps[0], &ga, 100 + run);
        PSquad *a = &play.sq[16], *e = &play.sq[6];
        a->archers = 10; a->knights = 0; a->barbarians = 0; a->spies = 0;
        e->archers = 0; e->knights = 7; e->barbarians = 7;
        squads_update(&play);
        a->x = e->x + 1; a->y = e->y + 3; play.time_div = 1;
        for (int f = 0; f < 50 && !a->fighting; f++) play_frame(&play, 0);
        cur_retreat = &bt.retreat;
        if (!battle_start(&bt, &play, 16)) continue;
        battles++;
        int last_ax[MAX_ARROWS], last_ay[MAX_ARROWS], lost0 = bt.lost_en;
        for (int k = 0; k < MAX_ARROWS; k++) last_ax[k] = bt.ax[k], last_ay[k] = bt.ay[k];
        for (int f = 0; f < 3000 && !bt.done; f++) {
            if (f == 2) battle_key(&bt, PK_A);
            int le = bt.lost_en;
            battle_frame(&bt); frames++;
            if (run == 2 && f % 6 == 0 && f <= 60) { char nm[32]; SDL_snprintf(nm, sizeof nm, "%s_f%02d_r%d", g ? "open" : "fort", f, bt.retreat); shot(argv[1], nm); }
            for (int k = 0; k < MAX_ARROWS; k++) {
                if (bt.aside[k] != FRIEND) { last_ax[k] = bt.ax[k]; last_ay[k] = bt.ay[k]; continue; }
                if (last_ax[k] == 9999 && bt.ax[k] != 9999) loosed++;
                if (last_ax[k] != 0 && last_ax[k] != 9999 && bt.ax[k] == 0) {
                    int x = last_ax[k] + rdb(&bt, O7(0x220) + bt.akind[k]), y = last_ay[k] + rdb(&bt, O7(0x235) + bt.akind[k]);
                    if (force_bg == 3 && x > ((y - 5) >> 3) + 60) wall++; else edge++;
                }
                if (last_ax[k] == 9999 && bt.ax[k] == 0) {     /* loosed and gone in the same frame */
                    loosed++;
                    if (force_bg == 3) wall++; else edge++;
                }
                last_ax[k] = bt.ax[k]; last_ay[k] = bt.ay[k];
            }
            (void)le;
        }
        enemy_lost += bt.lost_en - lost0;
        if (run < 4) SDL_Log("  bg %d run %d: lost me %d en %d retreat %d start %d", force_bg, run, bt.lost_me, bt.lost_en, bt.retreat, bt.start);
        if (run == 0) {
            SDL_Log("bg %d run 0: my archers at:", force_bg);
            for (int i = 0; i < MAX_MEN; i++) if (bt.addr[i] != -1 && bt.side[i] == FRIEND) SDL_Log("   x %d y %d", bt.x[i], bt.y[i]);
        }
        (void)hits_en;
      }
      SDL_Log("bg %d: arrows %ld (%ld after a retreat), arrow kills by my side %ld (%ld after a retreat), by theirs %ld", bgs[g], n_arrow, n_arrow_r, n_hit_en, n_hit_r, n_hit_me); n_arrow_r = n_hit_r = 0; n_arrow = n_hit_en = n_hit_me = 0;
      SDL_Log("bg %d: %ld battles, %ld frames, arrows loosed %ld, stopped at the wall %ld, other ends %ld, enemy men lost %ld",
              bgs[g], battles, frames, loosed, wall, edge, enemy_lost);
    }
    return 0;
}
