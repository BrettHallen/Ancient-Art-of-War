/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_demo.c: a test or debugging aid for the remake (see README.md here)
 */
/* The demo: the title page left alone, then the computer plays.
 * test_demo OUTDIR GAMEDIR DATADIR [runs] */
#include "../src/menu.h"
#include "../src/battle.h"
#include <SDL3/SDL.h>
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Screen scr; static uint32_t rgb[SCREEN_W * SCREEN_H];
static Menu *m; static const char *out;
static Play play; static Campaign game; static Battle bat;
static void save(const char *name)
{
    screen_to_xrgb(&scr, rgb);
    SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char p[512]; SDL_snprintf(p, sizeof p, "%s/%s.bmp", out, name);
    SDL_SaveBMP(s, p); SDL_DestroySurface(s);
}

/* one demo game from a campaign; returns frames played, sets *battle, *how */
static int run(const Campaign *c, uint32_t seed, bool shots, int *battle_frames, const char **how)
{
    game = *c;
    game.opponent = 0;
    game.rules[RULE_ENEMY_SEEN] = 80;
    play_start_demo(&play, &game, &ga, seed);
    play.fight_delay = 400;
    int f = 0, last_top = -1, scrolls = 0, ticks_frames = 0;
    *battle_frames = 0;
    *how = "running";
    for (; f < 20 * 60 * 30; f++) {
        if (play.zoom_squad >= 0) {
            if (!battle_start(&bat, &play, play.zoom_squad)) { *how = "no battle files"; break; }
            int bf = 0;
            while (!bat.done && bf < 8 * 60 * 10) {
                battle_frame(&bat);
                if (shots && (bf % 40 == 0) && bf < 400) {
                    battle_draw(&bat, &scr, MODE_CGA);
                    char n[64]; SDL_snprintf(n, sizeof n, "demo_battle_%03d", bf / 40); save(n);
                }
                bf++;
            }
            *battle_frames = bf;
            battle_finish(&bat);
            *how = bat.done ? "battle over" : "battle stuck";
            break;
        }
        int before = play.tick_count;
        play_frame(&play, 0);
        static int extra;                   /* as main.c: scrolling turns 2 1/2 times as fast */
        #define SCROLLING (play.demo == 1 && play.result == PLAY_RUNNING && play.zoom_squad < 0 && (play.demo_key == PK_KP_MINUS || play.demo_key == PK_KP_PLUS))
        if (SCROLLING) { extra += 3; while (extra >= 2 && SCROLLING) { extra -= 2; play_frame(&play, 0); } } else extra = 0;
        if (play.tick_count == 0 && play.demo_key == 0) ticks_frames++;
        if (play.view_top != last_top) { if (last_top >= 0 && (play.view_top == 0 || play.view_top == VIEW_TOP_MAX)) scrolls++; last_top = play.view_top; }
        if (shots && f % 100 == 0 && f < 1200) {
            play_draw(&play, &scr, MODE_CGA);
            char n[64]; SDL_snprintf(n, sizeof n, "demo_map_%04d", f); save(n);
        }
        if (play.result != PLAY_RUNNING) { *how = play.ended ? "game over" : "prompt"; break; }
    }
    if (shots) SDL_Log("  scrolls to an end %d, frames with a tick %d", scrolls, ticks_frames);
    return f;
}

int main(int argc, char **argv)
{
    (void)argc; char err[256];
    out = argv[1];
    if (!assets_load(&ga, argv[2], err, sizeof err)) { SDL_Log("%s", err); return 1; }
    campaigns_load(camps, argv[2]);
    int runs = argc > 4 ? SDL_atoi(argv[4]) : 50;

    /* the title page, left alone */
    m = menu_create(&ga, camps, argv[3]);
    menu_title(m);
    int f = 0; MenuAction a = MENU_NONE;
    for (; f < 20 * 60 * 10 && !a; f++) {
        menu_draw(m, &scr, MODE_CGA);
        menu_frame(m);
        a = menu_action(m);
        if (f == 20 * 60 * 10 - 1) break;
        if (f == 455) save("demo_title_before");
        if (f > 455 && f < 495 && (f % 4) == 0) { char n[64]; SDL_snprintf(n, sizeof n, "demo_title_%04d", f); save(n); }
    }
    SDL_Log("demo after %d frames (%.1f s), action %d (demo %d), campaign %s opp %d seen %d",
            f, f / 20.0, a, MENU_DEMO, menu_game(m)->file, menu_game(m)->opponent, menu_game(m)->rules[RULE_ENEMY_SEEN]);

    /* a key resets the wait */
    menu_title(m);
    for (int i = 0; i < 400; i++) { menu_draw(m, &scr, MODE_CGA); menu_frame(m); if (menu_action(m)) SDL_Log("early action!"); }
    menu_key(m, PK_EQUALS);
    int g = 0;
    for (a = MENU_NONE; !a && g < 20 * 60 * 10; g++) { menu_draw(m, &scr, MODE_CGA); menu_frame(m); a = menu_action(m); }
    SDL_Log("after a key at 20 s: demo %d frames later", g);

    /* one demo with pictures */
    int bf; const char *how;
    int n = run(menu_game(m), 1234, true, &bf, &how);
    SDL_Log("demo %s: %d map frames (%.1f s), %s, battle %d frames (%.1f s)", menu_game(m)->file, n, n / 20.0, how, bf, bf / 8.0);

    /* many */
    int total = 0, battles = 0, overs = 0, longest = 0;
    for (int r = 0; r < runs; r++) {
        const Campaign *c = &camps[3 + r % 8];
        if (!c->loaded) continue;
        n = run(c, (uint32_t)(r * 7919 + 13), false, &bf, &how);
        total++;
        if (!SDL_strcmp(how, "battle over")) battles++;
        else if (!SDL_strcmp(how, "game over")) overs++;
        else SDL_Log("  %s seed %d: %s after %d", c->file, r, how, n);
        if (n > longest) longest = n;
        if (r < 16) SDL_Log("  %s: %s after %.1f s on the map, battle %.1f s", c->file, how, n / 20.0, bf / 8.0);
    }
    SDL_Log("%d demos: %d ended with a battle, %d with the game over, longest on the map %.1f s", total, battles, overs, longest / 20.0);
    return 0;
}
