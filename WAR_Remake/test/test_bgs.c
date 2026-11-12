/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_bgs.c: a test or debugging aid for the remake (see README.md here)
 */
/* Every battle background with soldiers on it, a few seconds in. */
static int force_bg = -1;
#define BATTLE_FORCE_BG force_bg
#include "../src/play.c"
#include "../src/battle.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play play; static Battle bt; static Screen scr;
static uint32_t rgb[SCREEN_W * SCREEN_H];
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    if (!assets_load(&ga, argv[2], err, sizeof err)) return 1;
    campaigns_load(camps, argv[2]);
    for (int bg = 0; bg < 14; bg++) {
        force_bg = bg;
        play_start(&play, &camps[0], &ga, 7 + bg);
        PSquad *a = &play.sq[16], *e = &play.sq[6];
        a->archers = 4; a->knights = 5; a->barbarians = 5; e->archers = 5; e->knights = 5; e->barbarians = 4;
        squads_update(&play);
        a->x = e->x + 1; a->y = e->y + 3; play.time_div = 1;
        for (int f = 0; f < 50 && !a->fighting; f++) play_frame(&play, 0);
        battle_start(&bt, &play, 16);
        int stop = 24;
        for (int f = 0; f < stop; f++) {
            if (f == 2) battle_key(&bt, PK_A);
            if (f == 4) battle_key(&bt, PK_S);
            if (f == 6) battle_key(&bt, PK_A);
            battle_frame(&bt);
        }
        for (int m = 0; m < 2; m++) {
            battle_draw(&bt, &scr, m ? MODE_VGA : MODE_CGA); screen_to_xrgb(&scr, rgb);
            SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
            char p[512]; SDL_snprintf(p, sizeof p, "%s/bg%02d_%s.bmp", argv[1], bg, m ? "vga" : "cga"); SDL_SaveBMP(s, p); SDL_DestroySurface(s);
        }
        int f = 0; while (!bt.done && f < 3000) { battle_frame(&bt); f++; }
        SDL_Log("bg %2d: ended after %d more frames, lost %d/%d retreat %d", bg, f, bt.lost_me, bt.lost_en, bt.retreat);
    }
    return 0;
}
