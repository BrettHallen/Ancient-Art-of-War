/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_sound.c: a test or debugging aid for the remake (see README.md here)
 */
/* Render the remake's sounds to WAV files (no audio device): test_sound OUTDIR GAMEDIR */
#include "../src/sound.h"
#include "../src/assets.h"
#include <SDL3/SDL.h>
static GameAssets ga; static const char *out;
static void save(const char *name)
{
    int n = (int)(sound_busy() * 44100) + 4410;
    float *f = SDL_malloc(n * sizeof *f); int16_t *s = SDL_malloc(n * 2);
    sound_render(f, n);
    for (int i = 0; i < n; i++) s[i] = (int16_t)SDL_clamp(f[i] * 32767.0f, -32768, 32767);
    char p[512]; SDL_snprintf(p, sizeof p, "%s/%s.wav", out, name);
    SDL_IOStream *io = SDL_IOFromFile(p, "wb");
    uint32_t data = n * 2, riff = 36 + data, sr = 44100, br = 88200, fmt = 16; uint16_t pcm = 1, ch = 1, ba = 2, bits = 16;
    SDL_WriteIO(io, "RIFF", 4); SDL_WriteIO(io, &riff, 4); SDL_WriteIO(io, "WAVEfmt ", 8); SDL_WriteIO(io, &fmt, 4);
    SDL_WriteIO(io, &pcm, 2); SDL_WriteIO(io, &ch, 2); SDL_WriteIO(io, &sr, 4); SDL_WriteIO(io, &br, 4);
    SDL_WriteIO(io, &ba, 2); SDL_WriteIO(io, &bits, 2); SDL_WriteIO(io, "data", 4); SDL_WriteIO(io, &data, 4);
    SDL_WriteIO(io, s, data); SDL_CloseIO(io);
    SDL_Log("%s: %.3f s", name, n / 44100.0);
    SDL_free(f); SDL_free(s);
}
int main(int argc, char **argv)
{
    (void)argc; char err[256]; out = argv[1];
    if (!assets_load(&ga, argv[2], err, sizeof err)) return 1;
    sound_init_offline();
    sound_setting(SOUND_ON);
    snd_tune(ga.o2, 0x447, 0x48B, 0x110D, 0x1157, 0x119F, 0x11E7); save("start_pc");
    snd_tune(ga.o2, 0x122F, 0x12C7, 0x122F, 0x1263, 0x1295, 0x12C7); save("win_pc");
    snd_tune(ga.o2, 0x12F9, 0x133D, 0x12F9, 0x1311, 0x1327, 0x133D); save("lose_pc");
    static const unsigned BUGLES[4] = { 0x778, 0x792, 0x7A8, 0x7BE };
    for (int k = 0; k < 4; k++) { snd_bugle(ga.o7, BUGLES[k]); char n[32]; SDL_snprintf(n, sizeof n, "bugle_%x_pc", BUGLES[k]); save(n); }
    snd_beep(); snd_pause(5000); snd_beep_high(); snd_pause(5000); snd_buzz(); snd_pause(5000); snd_alert(); snd_pause(5000);
    snd_error(); snd_pause(5000); snd_kill(); snd_pause(5000); snd_arrow(); snd_pause(5000); snd_end_of_turn(); snd_pause(5000); snd_group_full();
    save("effects_pc");
    for (int i = 0; i < 46; i++) snd_scroll_step((i * 7919 % 128) < 95, 1.0 / 80);
    snd_scroll_end(); save("scroll_pc");
    sound_setting(SOUND_TANDY);
    snd_tune(ga.o2, 0x447, 0x48B, 0x110D, 0x1157, 0x119F, 0x11E7); save("start_tandy");
    snd_tune(ga.o2, 0x122F, 0x12C7, 0x122F, 0x1263, 0x1295, 0x12C7); save("win_tandy");
    snd_tune(ga.o2, 0x12F9, 0x133D, 0x12F9, 0x1311, 0x1327, 0x133D); save("lose_tandy");
    snd_bugle(ga.o7, 0x778); save("bugle_778_tandy");
    sound_setting(SOUND_NO_MUSIC);
    snd_tune(ga.o2, 0x447, 0x48B, 0x110D, 0x1157, 0x119F, 0x11E7); snd_bugle(ga.o7, 0x778); snd_end_of_turn();
    SDL_Log("no music: %.3f s queued (want 0)", sound_busy());
    snd_beep(); SDL_Log("no music, beep: %.4f s", sound_busy()); save("x");
    sound_setting(SOUND_OFF); snd_beep(); snd_alert(); SDL_Log("off: %.4f s", sound_busy());
    return 0;
}
