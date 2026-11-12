/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_battle.c: a test or debugging aid for the remake (see README.md here)
 */
/* Test harness: encounters on the map (capture, countdown, fighting it out). */
#include "../src/play.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play play; static Screen scr;
static uint32_t rgb[SCREEN_W * SCREEN_H]; static const char *outdir;
static void shot(const char *name)
{
    play_draw(&play, &scr, MODE_CGA); screen_to_xrgb(&scr, rgb);
    SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char p[512]; SDL_snprintf(p, sizeof p, "%s/%s.bmp", outdir, name); SDL_SaveBMP(s, p); SDL_DestroySurface(s);
}
static void rows(void) { for (int r = 25 - play.text_rows; r < 25; r++) SDL_Log("  %2d [%s]", r, play.text[r]); }
static void sq(int i) { const PSquad *q = &play.sq[i]; SDL_Log("  sq%d used %d fight %d foe %d clock %d kill %d men %d (a%d k%d b%d) cond %d", i, q->used, q->fighting, q->foe, q->clock, q->kill, q->men, q->archers, q->knights, q->barbarians, q->condition); }
int main(int argc, char **argv)
{
    outdir = argv[1]; char err[256];
    if (!assets_load(&ga, argv[2], err, sizeof err)) return 1;
    campaigns_load(camps, argv[2]);
    play_start(&play, &camps[0], &ga, 7);
    /* friendly archers (18) right next to enemy 6 (7 archers, 7 knights) */
    PSquad *a = &play.sq[18], *e = &play.sq[6];
    SDL_Log("strength 18 v 6: %d, edge %d", strength(&play, 18, 6), edge(&play, 18, 6));
    a->x = e->x + 1; a->y = e->y + 3;
    play.time_div = 1;
    play_frame(&play, 0);
    rows(); sq(18); sq(6);
    play.cx = a->x + 1; play.cy = a->y; check_over(&play, 1);
    rows(); shot("b01_encounter_zoom");
    int f;
    for (f = 0; f < 3000 && (a->fighting || e->fighting); f++) {
        play_frame(&play, 0);
        if (a->clock == FIGHT_ON && f % 50 == 0) SDL_Log("  frame %d: men %d v %d", f, a->men, e->men);
        if (f == 200) { rows(); shot("b02_fighting"); }
    }
    SDL_Log("after %d frames", f); rows(); sq(18); sq(6);
    shot("b03_result");
    SDL_Log("lost mine %d theirs %d", play.lost_mine, play.lost_theirs);
    /* capture: strong squad against a weak one */
    play_start(&play, &camps[0], &ga, 9);
    e = &play.sq[13]; e->archers = 1; e->knights = 0; squads_update(&play);
    a = &play.sq[16];
    SDL_Log("strength 16 v 13: %d", strength(&play, 16, 13));
    a->x = e->x; a->y = e->y + 2;
    play.time_div = 1; play_frame(&play, 0);
    rows(); sq(16); sq(13);
    SDL_Log("captured theirs %d", play.captured_theirs);
    shot("b04_captured");
    assets_free(&ga);
    return 0;
}
