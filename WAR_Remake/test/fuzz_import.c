/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fuzz_import.c: a test or debugging aid for the remake (see README.md here)
 */
/* damaged original S or SET files: import, resume and play (ASan) */
#include "../src/savegame.h"
#include "../src/sound.h"
#include <SDL3/SDL.h>
static GameAssets ga; static Campaign c; static Play p; static Screen scr;
int main(int argc, char **argv)
{
    (void)argc; char err[256]; sound_init_offline(); assets_load(&ga, argv[1], err, sizeof err);
    size_t n = 0; uint8_t *orig = SDL_LoadFile(argv[2], &n), s[ORIGINAL_SET_SIZE];   /* S or SET */
    uint32_t r = 1; int ok = 0, ended = 0;
    for (int it = 0; it < 400; it++) {
        SDL_memcpy(s, orig, n);
        int flips = 1 + it % 40;
        for (int k = 0; k < flips; k++) { r = r * 1103515245u + 12345u; s[(r >> 8) % n] = (uint8_t)(r >> 3); }
        if (!savegame_import(&p, &c, &ga, s, n, it)) continue;
        ok++;
        p.fight_delay = 400;
        play_resume(&p);
        for (int f = 0; f < 600 && p.result == PLAY_RUNNING; f++) {
            play_frame(&p, 0);
            if (p.mode == PM_PROMPT) play_key(&p, p.ended ? PK_ENTER : PK_N, false);
            if (f % 50 == 0) play_draw(&p, &scr, f % 100 ? MODE_CGA : MODE_VGA);
        }
        ended += p.result != PLAY_RUNNING;
    }
    SDL_Log("imported %d of 400 damaged files, %d games ended", ok, ended);
    return 0;
}
