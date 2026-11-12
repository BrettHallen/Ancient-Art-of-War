/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_datadisk.c: a test or debugging aid for the remake (see README.md here)
 */
/* A campaign made with the original's editor, on the data disk */
#include "../src/menu.h"
#include <SDL3/SDL.h>
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Screen scr; static uint32_t rgb[SCREEN_W * SCREEN_H];
static Menu *m;
static void frames(int n) { for (int i = 0; i < n; i++) { menu_draw(m, &scr, MODE_CGA); menu_frame(m); } }
static void key(int k) { menu_key(m, k); frames(2); }
static void shot(const char *path)
{
    menu_draw(m, &scr, MODE_CGA); screen_to_xrgb(&scr, rgb);
    SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    SDL_SaveBMP(s, path); SDL_DestroySurface(s);
}
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) return 1;
    campaigns_load(camps, argv[1]);
    m = menu_create(&ga, camps, argv[2]);
    menu_title(m); frames(2);
    key(PK_ENTER); frames(40);
    for (int i = 0; i < 12; i++) key(PK_EQUALS);
    key(PK_ENTER); frames(3);                      /* ( data disk ) */
    shot("menu/dd_titles.bmp");
    key(PK_EQUALS); key(PK_ENTER); frames(60);     /* My Campaign */
    shot("menu/dd_story.bmp");
    const Campaign *g = menu_game(m);
    SDL_Log("game file %s loaded %d opponent %d, story starts: %.40s", g->file, g->loaded, g->opponent, g->story);
    SDL_Log("T5 story starts:                          %.40s", camps[5].story);
    return 0;
}
