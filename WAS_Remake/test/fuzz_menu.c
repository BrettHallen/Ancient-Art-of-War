/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fuzz_menu.c: random keys at random moments through the menus (under
 * AddressSanitizer): whatever overlay 1 asks for is done as main.c does it
 * (a game played a little, then saved, quit or reported; the saved game
 * resumed; the demo), and every page is drawn in both modes.
 *
 * Usage: fuzz_menu GAME-FOLDER WORK-FOLDER [KEYS]
 */
#include <SDL3/SDL.h>

#include "../src/assets.h"
#include "../src/campaign.h"
#include "../src/menu.h"
#include "../src/play.h"

static uint32_t r = 1;
static int rnd(int n) { r = r * 1103515245u + 12345u; return (int)((r >> 16) % (unsigned)n); }

int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("usage: fuzz_menu GAME-FOLDER WORK-FOLDER [KEYS]"); return 2; }
    int keys = argc > 3 ? SDL_atoi(argv[3]) : 20000;
    static GameAssets ga;
    static Campaign camps[NUM_CAMPAIGNS], game;
    static Play p;
    static Screen scr;
    char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("assets: %s", err); return 2; }
    campaigns_load(camps, argv[1]);
    char data[1200];
    SDL_snprintf(data, sizeof data, "%s/", argv[2]);
    SDL_CreateDirectory(data);
    Menu *m = menu_create(&ga, camps, data);
    static const int KEYS[] = { PK_ENTER, PK_ENTER, PK_ENTER, PK_KP_PLUS, PK_KP_MINUS, PK_EQUALS, PK_MINUS, PK_ESC,
                                PK_SPACE, PK_S, PK_N, PK_C, 0x10, PK_Y, PK_T, PK_I };
    int games = 0, saves = 0, resumes = 0, demos = 0, reports = 0;
    menu_start(m);
    for (int k = 0; k < keys; k++) {
        int frames = rnd(4) == 0 ? rnd(600) : rnd(30);
        for (int f = 0; f < frames; f++) {
            menu_run(m, 1e6 / 60);
            MenuAction a = menu_action(m);
            uint16_t rr, ptr;
            menu_random(m, &rr, &ptr);
            bool play = false;
            if (a == MENU_START_GAME) { game = *menu_game(m); play_start_rnd(&p, &game, &ga, rr, ptr); play = true; games++; }
            else if (a == MENU_DEMO) { game = *menu_game(m); play_start_demo(&p, &game, &ga, rr, ptr); play = true; demos++; }
            else if (a == MENU_RESUME_GAME) { if (menu_resume(m, &p, &game)) { play = true; resumes++; } else menu_title(m); }
            else if (a == MENU_CONTINUE) { play_resume(&p); play = true; }
            else if (a == MENU_QUIT) menu_title(m);
            if (!play) continue;
            int n = rnd(3000);                   /* play a little */
            for (int i = 0; i < n && p.result == PLAY_RUNNING; i++) play_frame(&p, rnd(8) == 0 ? KEYS[rnd(16)] : 0);
            int w = rnd(4);
            if (p.result == PLAY_WON || p.result == PLAY_LOST || w == 0) { menu_report(m, &p); reports++; }
            else if (w == 1 && !p.demo) { menu_save(m, &p); saves++; }
            else menu_title(m);
        }
        menu_key(m, KEYS[rnd(16)]);
        if (rnd(2)) menu_key_up(m);
        if (k % 50 == 0) { menu_draw(m, &scr, MODE_CGA); menu_draw(m, &scr, MODE_16); }
    }
    SDL_Log("fuzz_menu: %d keys; %d games, %d saved, %d resumed, %d demos, %d reports", keys, games, saves, resumes, demos, reports);
    menu_destroy(m);
    assets_free(&ga);
    return 0;
}
