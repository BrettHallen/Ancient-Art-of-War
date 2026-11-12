/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_menu.c: a test or debugging aid for the remake (see README.md here)
 */
/* Walk the menus like a player and screenshot each page (CGA and VGA). */
#include "../src/menu.h"
#include <SDL3/SDL.h>
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Screen scr; static uint32_t rgb[SCREEN_W * SCREEN_H];
static Menu *m; static const char *out; static GfxMode mode;
static void shot(const char *name)
{
    menu_draw(m, &scr, mode); screen_to_xrgb(&scr, rgb);
    SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char p[512]; SDL_snprintf(p, sizeof p, "%s/%s_%s.bmp", out, name, mode == MODE_CGA ? "cga" : "vga");
    SDL_SaveBMP(s, p); SDL_DestroySurface(s);
}
static void frames(int n) { for (int i = 0; i < n; i++) { menu_draw(m, &scr, mode); menu_frame(m); MenuAction a = menu_action(m); if (a) SDL_Log("action %d", a); } }
static void key(int k) { menu_key(m, k); frames(2); }
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    out = argv[1];
    if (!assets_load(&ga, argv[2], err, sizeof err)) return 1;
    campaigns_load(camps, argv[2]);
    for (int md = 0; md < 2; md++) {
        mode = (GfxMode)md;
        m = menu_create(&ga, camps, argv[3]);
        menu_start(m); frames(3); shot("01_presented");
        frames(90); shot("02_title");
        frames(41); shot("03_title_blink");
        key(PK_EQUALS); shot("04_title_changes");
        key(PK_MINUS); key(PK_ENTER); frames(6); shot("05_titles_unrolling");
        frames(30); shot("06_titles");
        for (int i = 0; i < 5; i++) key(PK_EQUALS);
        shot("07_titles_down");
        for (int i = 0; i < 5; i++) key(PK_MINUS);
        key(PK_ENTER); frames(10); shot("08_rollup");
        frames(40); shot("09_story");
        for (int i = 0; i < 30; i++) key(PK_EQUALS);
        shot("10_story_down");
        key(PK_ENTER); frames(60); shot("11_rules");
        key(PK_EQUALS); key(PK_SPACE); shot("12_rules_changed");
        key(PK_ENTER); frames(60); shot("13_opponent");
        frames(25); shot("14_opponent_frame");
        key(PK_EQUALS); frames(3); shot("15_opponent_sweep");
        frames(30); shot("16_opponent_next");
        key(PK_ENTER); frames(2); shot("17_quote");
        const Campaign *g = menu_game(m);
        SDL_Log("game %s opponent %d rules %d %d", g->file, g->opponent, g->rules[0], g->rules[1]);
        menu_title(m); frames(3);
        key(PK_EQUALS); key(PK_ENTER); frames(3); shot("18_campaign_menu");
        frames(25); shot("19_campaign_menu_frame");
        for (int i = 0; i < 5; i++) { key(PK_EQUALS); frames(25); }
        shot("20_campaign_menu_shoppe");
        key(PK_ENTER); frames(2); shot("21_shoppe");
        key(PK_EQUALS); key(PK_SPACE); shot("22_shoppe_changed");
        key(PK_ENTER); frames(2); shot("23_title_again");
        menu_destroy(m);
    }
    assets_free(&ga);
    return 0;
}
