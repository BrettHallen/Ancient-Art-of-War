/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_import.c: a test or debugging aid for the remake (see README.md here)
 */
/* Load an original S, draw it, then play on (ASan) */
#include "../src/menu.h"
#include "../src/play.h"
#include "../src/battle.h"
#include "../src/sound.h"
#include <SDL3/SDL.h>
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS], game; static Screen scr; static uint32_t rgb[64000]; static Play p;
static void shot(const char *f) { play_draw(&p, &scr, MODE_CGA); screen_to_xrgb(&scr, rgb);
  SDL_Surface *s = SDL_CreateSurfaceFrom(320, 200, SDL_PIXELFORMAT_XRGB8888, rgb, 1280); SDL_SaveBMP(s, f); SDL_DestroySurface(s); }
int main(int argc, char **argv)
{
    (void)argc; char err[256]; sound_init_offline();
    assets_load(&ga, argv[1], err, sizeof err); campaigns_load(camps, argv[1]);
    Menu *m = menu_create(&ga, camps, argv[2]);
    SDL_Log("has saved game %d", menu_has_saved_game(m));
    if (!menu_load_saved_game(m, &p, &game)) { SDL_Log("load failed"); return 1; }
    p.fight_delay = 400;
    play_resume(&p);
    SDL_Log("title '%s' opponent %d time %d view %d cursor %d,%d sel %d", game.title, game.opponent, p.time_div, p.view_top, p.cx, p.cy, p.sel);
    for (int i = 0; i < MAX_SQUADS; i++) if (p.sq[i].used)
        SDL_Log("sq %2d %s at %2d,%3d men %2d (a%d k%d b%d s%d) cond %2d food %2d moving %d path %d@%d route %d@%d mark %d fight %d/%d clock %d",
            i, p.sq[i].friendly ? "F" : "E", p.sq[i].x, p.sq[i].y, p.sq[i].men, p.sq[i].archers, p.sq[i].knights, p.sq[i].barbarians, p.sq[i].spies,
            p.sq[i].condition, p.sq[i].food, p.sq[i].moving, p.sq[i].has_path, p.sq[i].path_pos, p.sq[i].on_route, p.sq[i].route_pos, p.sq[i].marked,
            p.sq[i].fighting, p.sq[i].foe, p.sq[i].clock);
    shot(argv[3]);
    static Battle b; long frames = 0; int battles = 0;
    for (; frames < 200000 && p.result == PLAY_RUNNING; frames++) {
        play_frame(&p, 0);
        if (p.zoom_squad >= 0) { battles++; p.zoom_squad = -1; }
        if (p.mode == PM_PROMPT) play_key(&p, p.ended ? PK_ENTER : PK_N, false);
    }
    (void)b;
    SDL_Log("played %ld frames: result %d code %d, lost %d/%d", frames, p.result, p.result_code, p.lost_mine, p.lost_theirs);
    return 0;
}
