/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_groups.c: a test or debugging aid for the remake (see README.md here)
 */
/* Formation, Detach and Join on the map, with screenshots */
#include "../src/play.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play play; static Screen scr;
static uint32_t rgb[SCREEN_W * SCREEN_H]; static const char *out;
static void shot(const char *name)
{
    play_draw(&play, &scr, MODE_CGA); screen_to_xrgb(&scr, rgb);
    SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char p[512]; SDL_snprintf(p, sizeof p, "%s/%s.bmp", out, name); SDL_SaveBMP(s, p); SDL_DestroySurface(s);
}
static void key(int k) { play_key(&play, k, false); play_frame(&play, 0); }
static void dump(const char *t) { SDL_Log("%s: mode %d rows %d", t, play.mode, play.text_rows); for (int r = 25 - play.text_rows; r < 25; r++) SDL_Log("  %2d [%s]", r, play.text[r]); }
int main(int argc, char **argv)
{
    (void)argc; char err[256]; out = argv[1];
    if (!assets_load(&ga, argv[2], err, sizeof err)) return 1;
    campaigns_load(camps, argv[2]);
    play_start(&play, &camps[0], &ga, 1);
    int sel = -1;
    for (int i = 0; i < MAX_SQUADS; i++) if (play.sq[i].used && play.sq[i].friendly && play.sq[i].archers) { sel = i; break; }
    PSquad *q = &play.sq[sel];
    SDL_Log("squad %d at %d,%d: a%d k%d b%d s%d form %d", sel, q->x, q->y, q->archers, q->knights, q->barbarians, q->spies, q->formation);
    play.cx = q->x + 1; play.cy = q->y + 1; check_over(&play, 1);
    play_frame(&play, 0);
    dump("over squad");
    key(PK_F); dump("formation"); shot("g1_formation");
    key(PK_EQUALS); key(PK_EQUALS); shot("g2_formation3");
    q->knights = 3; q->barbarians = 4; q->archers = 7; count_men(&play);
    key(PK_EQUALS); shot("g3_formation4_mixed");
    key(PK_E); dump("after E");
    q->knights = 0; q->barbarians = 0; q->archers = 14; count_men(&play);
    key(PK_D); dump("detach"); shot("g4_detach");
    key(PK_A); key(PK_A); dump("two archers"); shot("g5_detach2");
    key(PK_E); dump("end");
    int n = -1;
    for (int i = 0; i < MAX_SQUADS; i++) if (play.sq[i].used && i != sel && play.sq[i].x == q->x + 1 && play.sq[i].y == q->y) n = i;
    SDL_Log("new squad %d: men %d archers %d, old men %d; partner %d", n, n >= 0 ? play.sq[n].men : -1, n >= 0 ? play.sq[n].archers : -1, q->men, play.partner);
    shot("g6_after");
    play_frame(&play, 0);
    dump("cmd line");
    key(PK_J); dump("join"); shot("g7_join");
    key(PK_A); dump("one back");
    key(PK_D); dump("switch to detach");
    key(PK_E);
    SDL_Log("after: old men %d new men %d", q->men, n >= 0 ? play.sq[n].men : -1);
    return 0;
}
