/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fuzz_formed.c: a test or debugging aid for the remake (see README.md here)
 */
/* random keys and held keys in the formation editor (ASan) */
#include "../src/formed.h"
#include "../src/play.h"
#include "../src/sound.h"
#include <SDL3/SDL.h>
static GameAssets ga; static Screen scr;
int main(int argc, char **argv)
{
    (void)argc; char err[256]; sound_init_offline();
    assets_load(&ga, argv[1], err, sizeof err);
    static const int K[] = { PK_ENTER, PK_M, PK_E, PK_S, PK_A, PK_B, PK_K, PK_Y, PK_N, PK_EQUALS, PK_MINUS, PK_KP_PLUS, PK_UP, PK_LEFT };
    uint32_t r = 7; FormEdit e; long saves = 0, games = 0, modes[4] = { 0 };
    formed_start(&e, &ga, ga.formations);
    for (long i = 0; i < 2000000; i++) {
        r = r * 1103515245u + 12345u;
        int pick = (r >> 16) % 100;
        if (pick < 8) formed_key(&e, K[(r >> 8) % (sizeof K / sizeof K[0])]);
        int held = pick > 40 ? PK_HOME + (int)((r >> 4) % 11) : 0;
        formed_frame(&e, held);
        if (i % 97 == 0) formed_draw(&e, &scr, (r >> 20) % 2 ? MODE_VGA : MODE_CGA);
        if (e.save) { formed_sort(e.f); SDL_memcpy(ga.formations, e.f, 420); e.save = false; saves++; }
        modes[e.mode & 3]++;
        if (e.done) { games++; formed_start(&e, &ga, ga.formations); }
        for (int k = 0; k < 14; k++)
            if (e.x[k] < 0x20 || e.x[k] > 0x52 || e.y[k] < 0x10 || e.y[k] > 0x50) { SDL_Log("man %d out at %d,%d", k, e.x[k], e.y[k]); return 1; }
    }
    SDL_Log("saves %ld, exits %ld, modes %ld %ld %ld", saves, games, modes[1], modes[2], modes[3]);
    return 0;
}
