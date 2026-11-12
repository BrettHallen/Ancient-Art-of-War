/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fuzz_play.c: random keys in campaign play, every campaign, under
 * AddressSanitizer (build.sh). Checks that the marker and the view stay in
 * their limits and that every frame draws.
 *
 * Usage: fuzz_play GAME-FOLDER [FRAMES-PER-CAMPAIGN]
 */
#include <SDL3/SDL.h>

#include "../src/assets.h"
#include "../src/campaign.h"
#include "../src/play.h"

static uint32_t seed = 4321;
static uint32_t rnd(void) { seed = seed * 1103515245u + 12345u; return seed >> 8; }

static const int KEYS[] = {
    PK_ESC, PK_MINUS, PK_EQUALS, PK_TAB, PK_E, PK_T, PK_Y, PK_I, PK_P, PK_ENTER, PK_CTRL, PK_S, PK_D, PK_F,
    PK_J, PK_Z, PK_C, PK_N, PK_M, PK_SPACE, PK_HOME, PK_UP, PK_PGUP, PK_KP_MINUS, PK_LEFT, PK_KP_5, PK_RIGHT,
    PK_KP_PLUS, PK_END, PK_DOWN, PK_PGDN, PK_ALT_S, 0x77,
};

int main(int argc, char **argv)
{
    if (argc < 2) { SDL_Log("usage: fuzz_play GAME-FOLDER [FRAMES]"); return 2; }
    int frames = argc > 2 ? SDL_atoi(argv[2]) : 20000;
    static GameAssets ga;
    static Campaign cs[NUM_CAMPAIGNS];
    static Play p;
    static Screen s;
    char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("load: %s", err); return 1; }
    campaigns_load(cs, argv[1]);
    int bad = 0, ended = 0;
    for (int c = 0; c < NUM_CAMPAIGNS; c++) {
        play_start(&p, &cs[c], &ga);
        for (int f = 0; f < frames; f++) {
            if (p.result != PLAY_RUNNING) { ended++; play_start(&p, &cs[c], &ga); }
            uint32_t r = rnd() % 10;
            if (r < 3) play_key(&p, KEYS[rnd() % (sizeof KEYS / sizeof KEYS[0])], rnd() % 4 == 0);
            int held = (r >= 3 && r < 8) ? PK_HOME + (int)(rnd() % 11) : 0;
            play_frame(&p, held);
            if (f % 7 == 0) play_draw(&p, &s, (GfxMode)(f % 2));
            if (p.cx < 1 || p.cx > CX_MAX || p.cy < 0 || p.cy > CY_MAX || p.view_top < 0 || p.view_top > VIEW_TOP_MAX ||
                p.cy < p.view_top || p.cy > p.view_top + VIEW_MARGIN || p.text_rows < 2 || p.text_rows > 25) {
                if (bad++ < 10) SDL_Log("S%d frame %d: marker %d,%d view %d rows %d", c, f, p.cx, p.cy, p.view_top, p.text_rows);
            }
        }
    }
    assets_free(&ga);
    SDL_Log("fuzz_play: %d frames a campaign, %d games ended, %d problems", frames, ended, bad);
    return bad ? 1 : 0;
}
