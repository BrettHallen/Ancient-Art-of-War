/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_menu2.c: a test or debugging aid for the remake (see README.md here)
 */
/* Save, resume, surrender and the battle report, through the menus. Plus random keys. */
#include "../src/menu.h"
#include <SDL3/SDL.h>
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Screen scr; static uint32_t rgb[SCREEN_W * SCREEN_H];
static Menu *m; static const char *out; static GfxMode mode;
static Play play, play2; static Campaign game, game2;
static void shot(const char *name)
{
    menu_draw(m, &scr, mode); screen_to_xrgb(&scr, rgb);
    SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char p[512]; SDL_snprintf(p, sizeof p, "%s/%s_%s.bmp", out, name, mode == MODE_CGA ? "cga" : "vga");
    SDL_SaveBMP(s, p); SDL_DestroySurface(s);
}
static MenuAction last;
static void frames(int n) { for (int i = 0; i < n; i++) { menu_draw(m, &scr, mode); menu_frame(m); MenuAction a = menu_action(m); if (a) last = a; } }
static void key(int k) { menu_key(m, k); frames(2); }
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    out = argv[1];
    if (!assets_load(&ga, argv[2], err, sizeof err)) return 1;
    campaigns_load(camps, argv[2]);
    mode = MODE_CGA;
    m = menu_create(&ga, camps, argv[3]);
    menu_title(m); frames(2);
    key(PK_ENTER); frames(40);                    /* go to war: titles */
    key(PK_EQUALS); key(PK_EQUALS); key(PK_ENTER); frames(40);   /* Contest of the Gods */
    key(PK_ENTER); frames(40); key(PK_ENTER); frames(40);       /* story, rules */
    key(PK_ENTER); frames(2); key(PK_ENTER); frames(2);          /* opponent, quote */
    SDL_Log("action %d game %s opp %d", last, menu_game(m)->file, menu_game(m)->opponent);
    game = *menu_game(m);
    play_start(&play, &game, &ga, menu_seed(m));
    play.time_div = 1;
    for (int f = 0; f < 200; f++) play_frame(&play, 0);
    play_key(&play, PK_ALT_S, false); play_frame(&play, 0);
    SDL_Log("result %d (save %d)", play.result, PLAY_SAVE);
    menu_save(m, &play); frames(2); shot("30_save");
    if (menu_has_saved_game(m)) { key(PK_C); frames(2); }
    frames(15); shot("31_saved");
    last = 0; key(PK_C);
    SDL_Log("after C: action %d (continue %d), saved %d", last, MENU_CONTINUE, menu_has_saved_game(m));
    play_resume(&play);
    for (int f = 0; f < 50; f++) play_frame(&play, 0);
    bool ok = menu_load_saved_game(m, &play2, &game2);
    SDL_Log("load: %d, ticks %d vs %d, squads x %d/%d, campaign %s", ok, play2.ticks, play.ticks, play2.sq[0].x, play.sq[0].x, play2.c->file);
    play_resume(&play2);
    for (int f = 0; f < 300; f++) play_frame(&play2, 0);
    SDL_Log("resumed game runs: ticks %d result %d", play2.ticks, play2.result);
    /* surrender, report */
    play_key(&play, PK_S, false); play_frame(&play, 0);
    play_key(&play, PK_Y, false); play_frame(&play, 0);
    play_key(&play, PK_ENTER, false); play_frame(&play, 0);
    SDL_Log("surrender: result %d code %d", play.result, play.result_code);
    menu_report(m, &play); frames(2); shot("32_report_lost");
    play.result_code = 0; play.lost_theirs = 7; play.lost_mine = 123; play.flags_captured = 2;
    menu_report(m, &play); frames(2); shot("33_report_won");
    mode = MODE_VGA; shot("34_report_won");
    /* data disk entry and the saved game question */
    menu_title(m); frames(2); key(PK_ENTER); frames(40);
    for (int i = 0; i < 12; i++) key(PK_EQUALS);
    key(PK_ENTER); frames(3); shot("35_saved_or_new");
    last = 0; key(PK_S); SDL_Log("S: action %d (resume %d)", last, MENU_RESUME_GAME);
    /* random keys */
    static const int keys[] = { PK_ENTER, PK_ESC, PK_EQUALS, PK_MINUS, PK_SPACE, PK_S, PK_N, PK_C, PK_Q, PK_Y, PK_UP, PK_DOWN };
    uint32_t r = 7; int acts[8] = { 0 };
    menu_title(m);
    for (int i = 0; i < 200000; i++) {
        r = r * 1103515245 + 12345;
        if ((r >> 16) % 3 == 0) menu_key(m, keys[(r >> 20) % 12]);
        if (i % 9000 == 0) mode = (GfxMode)((r >> 24) & 1);
        menu_draw(m, &scr, mode); menu_frame(m);
        MenuAction a = menu_action(m);
        if (a) { acts[a]++; if (a == MENU_QUIT || a == MENU_START_GAME || a == MENU_RESUME_GAME || a == MENU_CONTINUE) menu_title(m); }
    }
    SDL_Log("random keys: start %d resume %d continue %d quit %d", acts[MENU_START_GAME], acts[MENU_RESUME_GAME], acts[MENU_CONTINUE], acts[MENU_QUIT]);
    menu_destroy(m);
    assets_free(&ga);
    return 0;
}
