/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fuzz_assets.c: damaged game files must not crash the loader.
 *
 * Copies the game to a work folder, then again and again damages one file
 * (flips bytes, cuts it short, or makes it empty), loads everything and draws
 * every viewer screen. Run under AddressSanitizer (build.sh).
 *
 * Usage: fuzz_assets GAME-FOLDER WORK-FOLDER [ROUNDS]
 */
#include <SDL3/SDL.h>

#include "../src/assets.h"
#include "../src/campaign.h"
#include "../src/viewer.h"

static const char *FILES[] = {
    "WAS.EXE", "SM", "SPM", "SPI", "SPE", "SPF", "SPJ", "SPG", "SPH", "SPK", "SPN", "SPO", "SPP", "SPQ",
    "AD", "B", "WAST.DAT", "SITLES", "S0", "S1", "S2", "S3", "S4", "S5", "S6", "S7", "S8", "S9", "SA",
};
#define NFILES ((int)(sizeof FILES / sizeof FILES[0]))

static uint32_t seed = 12345;
static uint32_t rnd(void) { seed = seed * 1103515245u + 12345u; return seed >> 8; }

static bool copy_file(const char *from_dir, const char *to_dir, const char *name, void **keep, size_t *keep_n)
{
    char p[1200];
    SDL_snprintf(p, sizeof p, "%s/%s", from_dir, name);
    size_t n = 0;
    void *d = SDL_LoadFile(p, &n);
    if (!d) return false;
    SDL_snprintf(p, sizeof p, "%s/%s", to_dir, name);
    bool ok = SDL_SaveFile(p, d, n);
    *keep = d;
    *keep_n = n;
    return ok;
}

int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("usage: fuzz_assets GAME-FOLDER WORK-FOLDER [ROUNDS]"); return 2; }
    int rounds = argc > 3 ? SDL_atoi(argv[3]) : 600;
    SDL_CreateDirectory(argv[2]);
    void *orig[NFILES];
    size_t orig_n[NFILES];
    for (int f = 0; f < NFILES; f++)
        if (!copy_file(argv[1], argv[2], FILES[f], &orig[f], &orig_n[f])) { SDL_Log("can't copy %s", FILES[f]); return 1; }

    static GameAssets ga;
    static Campaign cs[NUM_CAMPAIGNS];
    static Screen s;
    int loaded = 0, refused = 0;
    for (int r = 0; r < rounds; r++) {
        int f = (int)(rnd() % NFILES);
        size_t n = orig_n[f];
        uint8_t *d = SDL_malloc(n + 1);
        SDL_memcpy(d, orig[f], n);
        switch (rnd() % 4) {
        case 0: for (int k = 0; k < 1 + (int)(rnd() % 40); k++) d[rnd() % n] = (uint8_t)rnd(); break;
        case 1: n = rnd() % n; break;
        case 2: n = 0; break;
        default: for (size_t k = rnd() % n; k < n; k++) d[k] = (uint8_t)rnd(); break;
        }
        char p[1200];
        SDL_snprintf(p, sizeof p, "%s/%s", argv[2], FILES[f]);
        SDL_SaveFile(p, d, n);
        SDL_free(d);

        char err[256];
        if (assets_load(&ga, argv[2], err, sizeof err)) {
            loaded++;
            campaigns_load(cs, argv[2]);
            Viewer v;
            viewer_init(&v, &ga, cs);
            for (int m = 0; m < MODE_COUNT; m++) {
                for (int i = 0; i < NUM_SCREENS; i++) {
                    viewer_draw(&v, &s);
                    if (viewer_is_map(&v)) for (int p = 0; p < 3; p++) { viewer_key(&v, VK_STORY); viewer_draw(&v, &s); }
                    viewer_key(&v, VK_NEXT);
                }
                viewer_key(&v, VK_MODE);
            }
            assets_free(&ga);
        } else {
            refused++;
        }
        SDL_SaveFile(p, orig[f], orig_n[f]);   /* put it back */
    }
    for (int f = 0; f < NFILES; f++) SDL_free(orig[f]);
    SDL_Log("fuzz_assets: %d rounds, %d loaded, %d refused", rounds, loaded, refused);
    return 0;
}
