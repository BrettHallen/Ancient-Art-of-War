/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/tempo.c: a test or debugging aid for the remake (see README.md here)
 */
#include "../src/sound.h"
#include "../src/assets.h"
#include <SDL3/SDL.h>
static GameAssets ga;
static void save(const char *path)
{
    int n = (int)(sound_busy() * 44100) + 8820;
    float *f = SDL_malloc(n * sizeof *f); int16_t *s = SDL_malloc(n * 2);
    sound_render(f, n);
    for (int i = 0; i < n; i++) s[i] = (int16_t)SDL_clamp(f[i] * 32767.0f, -32768, 32767);
    SDL_IOStream *io = SDL_IOFromFile(path, "wb");
    uint32_t data = n * 2, riff = 36 + data, sr = 44100, br = 88200, fmt = 16; uint16_t pcm = 1, ch = 1, ba = 2, bits = 16;
    SDL_WriteIO(io, "RIFF", 4); SDL_WriteIO(io, &riff, 4); SDL_WriteIO(io, "WAVEfmt ", 8); SDL_WriteIO(io, &fmt, 4);
    SDL_WriteIO(io, &pcm, 2); SDL_WriteIO(io, &ch, 2); SDL_WriteIO(io, &sr, 4); SDL_WriteIO(io, &br, 4);
    SDL_WriteIO(io, &ba, 2); SDL_WriteIO(io, &bits, 2); SDL_WriteIO(io, "data", 4); SDL_WriteIO(io, &data, 4);
    SDL_WriteIO(io, s, data); SDL_CloseIO(io); SDL_free(f); SDL_free(s);
    SDL_Log("%s %.2f s", path, n / 44100.0);
}
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    assets_load(&ga, argv[1], err, sizeof err);
    sound_init_offline(); sound_setting(SOUND_ON);
    static const double US[3] = { 19, 32, 50 };
    static const char *NAME[3] = { "1_now_like_DOSBox", "2_about_1.7x_slower", "3_IBM_PC_XT_2.6x_slower" };
    for (int k = 0; k < 3; k++) {
        sound_unit(US[k]);
        snd_tune(ga.o2, 0x447, 0x48B, 0x110D, 0x1157, 0x119F, 0x11E7);
        char p[256]; SDL_snprintf(p, sizeof p, "%s/Opening_tune_%s.wav", argv[2], NAME[k]); save(p);
    }
}
