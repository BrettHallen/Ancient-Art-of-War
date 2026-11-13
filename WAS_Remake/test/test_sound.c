/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_sound.c: step 8, the PC speaker rendered without a device
 * (sound_init_offline), in step with the clocks that make the sounds:
 * "presented by"'s tune, the title page's beeps, the scroll unrolling,
 * the battle report's tunes (won, lost), the opening, the map (a new
 * game's three low notes, a key's beep, the buzz over a squadron, a
 * message's note), a battle's first minute, and the sounds off and "do not
 * play music". Each is written as a WAV (mono, 44100 Hz) to listen to;
 * checks the notes' pitches (timer 2's divisor) and lengths (50 us a delay
 * unit), and that nothing sounds with the sounds off.
 *
 * Usage: test_sound GAME-FOLDER OUT-FOLDER
 */
#include <SDL3/SDL.h>
#include <stdarg.h>

#include "../src/assets.h"
#include "../src/battle.h"
#include "../src/campaign.h"
#include "../src/menu.h"
#include "../src/play.h"
#include "../src/sound.h"

static int fails = 0;
static void fail(int line, const char *fmt, ...)
{
    char msg[256];
    va_list ap;
    va_start(ap, fmt);
    SDL_vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    fails++;
    SDL_Log("FAIL line %d: %s", line, msg);
}
#define CHECK(c, ...) do { if (!(c)) fail(__LINE__, __VA_ARGS__); } while (0)

static GameAssets ga;
static Campaign camps[NUM_CAMPAIGNS];
static const char *outdir;

/* what has been rendered since the last wav_start */
static float *pcm;
static size_t npcm, cap;

static void render(double seconds)
{
    int n = (int)(seconds * SOUND_RATE + 0.5);
    if (npcm + (size_t)n > cap) {
        cap = (npcm + (size_t)n) * 2;
        pcm = SDL_realloc(pcm, cap * sizeof *pcm);
    }
    sound_render(pcm + npcm, n);
    npcm += (size_t)n;
}

static void wav_start(void) { npcm = 0; }

static void wav_save(const char *name)
{
    char path[1200];
    SDL_snprintf(path, sizeof path, "%s/%s.wav", outdir, name);
    SDL_IOStream *f = SDL_IOFromFile(path, "wb");
    CHECK(f != NULL, "can't write %s", path);
    if (!f) return;
    uint32_t data = (uint32_t)(npcm * 2);
    uint8_t h[44] = { 'R', 'I', 'F', 'F' };
    SDL_memcpy(h + 8, "WAVEfmt ", 8);
    uint32_t v32;
    uint16_t v16;
    v32 = 36 + data; SDL_memcpy(h + 4, &v32, 4);
    v32 = 16; SDL_memcpy(h + 16, &v32, 4);
    v16 = 1; SDL_memcpy(h + 20, &v16, 2);
    v16 = 1; SDL_memcpy(h + 22, &v16, 2);
    v32 = SOUND_RATE; SDL_memcpy(h + 24, &v32, 4);
    v32 = SOUND_RATE * 2; SDL_memcpy(h + 28, &v32, 4);
    v16 = 2; SDL_memcpy(h + 32, &v16, 2);
    v16 = 16; SDL_memcpy(h + 34, &v16, 2);
    SDL_memcpy(h + 36, "data", 4);
    SDL_memcpy(h + 40, &data, 4);
    SDL_WriteIO(f, h, 44);
    for (size_t i = 0; i < npcm; i++) {
        float x = pcm[i] * 2.0f;
        int16_t s = (int16_t)SDL_clamp(x * 32767.0f, -32767.0f, 32767.0f);
        SDL_WriteIO(f, &s, 2);
    }
    SDL_CloseIO(f);
}

/* how loud: the largest sample since sample a */
static float peak(size_t a, size_t b)
{
    float m = 0;
    for (size_t i = a; i < b && i < npcm; i++) m = SDL_max(m, SDL_fabsf(pcm[i]));
    return m;
}

/* the pitch from upward zero crossings between samples a and b */
static double pitch(size_t a, size_t b)
{
    int n = 0;
    size_t first = 0, last = 0;
    for (size_t i = a + 1; i < b && i < npcm; i++)
        if (pcm[i - 1] < 0 && pcm[i] >= 0) { if (!n) first = i; last = i; n++; }
    return n > 1 ? (double)(n - 1) * SOUND_RATE / (double)(last - first) : 0;
}

/* the first sample from a on that sounds, and the sounding stretches */
static size_t first_sound(size_t a)
{
    for (size_t i = a; i < npcm; i++) if (SDL_fabsf(pcm[i]) > 0.02f) return i;
    return npcm;
}

static void run_menu(Menu *m, double seconds)
{
    for (double t = 0; t < seconds; t += 1.0 / 60) {
        menu_run(m, 1e6 / 60);
        render(1.0 / 60);
        menu_action(m);
    }
}

static void key(Menu *m, int k)
{
    menu_key(m, k);
    run_menu(m, 0.1);
    menu_key_up(m);
    run_menu(m, 0.1);
}

static void set_sound(int v) { ga.scnf[0x684F - 0x684C] = (uint8_t)v; ga.scnf[0x6850 - 0x684C] = 0; }

int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("usage: test_sound GAME-FOLDER OUT-FOLDER"); return 2; }
    outdir = argv[1 + 1];
    char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("assets: %s", err); return 2; }
    campaigns_load(camps, argv[1]);
    CHECK(sound_init_offline(), "sound_init_offline");
    sound_options(ga.scnf);
    char data[1200];
    SDL_snprintf(data, sizeof data, "%s/sound_data/", outdir);
    SDL_CreateDirectory(data);
    set_sound(2);
    Menu *m = menu_create(&ga, camps, data);
    set_sound(2);

    /* "presented by": play_tune DS:0C1A / 0C86, divisor and 4 x units a note */
    wav_start();
    menu_start(m);
    run_menu(m, 9);
    wav_save("presented_tune");
    size_t s0 = first_sound(0);
    int d0 = ga.ds[0xC1A] | (ga.ds[0xC1B] << 8), u0 = ga.ds[0xC86] | (ga.ds[0xC87] << 8);
    double want = 1193182.0 / d0, got = pitch(s0 + 200, s0 + (size_t)(u0 * 4 * 50e-6 * SOUND_RATE) - 200);
    SDL_Log("presented by: first note %.1f Hz (divisor %d, %.0f Hz), %d units", got, d0, want, u0 * 4);
    CHECK(SDL_fabs(got - want) < want * 0.03, "first note %.1f Hz, not %.1f", got, want);
    double total = 0;
    for (int i = 0; ga.ds[0xC1A + 2 * i] | ga.ds[0xC1B + 2 * i]; i++) total += (ga.ds[0xC86 + 2 * i] | (ga.ds[0xC87 + 2 * i] << 8)) * 4 * 50e-6;
    size_t end = npcm;
    while (end > 0 && SDL_fabsf(pcm[end - 1]) < 0.02f) end--;
    double len = (double)(end - s0) / SOUND_RATE;
    SDL_Log("presented by: the tune %.2f s (the notes' delays %.2f s)", len, total);
    CHECK(SDL_fabs(len - total) < 0.1, "tune %.2f s, not %.2f", len, total);

    /* the title page: + and - (beep_low: divisor 500, 400 units) */
    wav_start();
    key(m, PK_KP_PLUS);
    run_menu(m, 0.5);
    wav_save("title_beep");
    size_t b0 = first_sound(0), b1 = b0;
    while (b1 < npcm && peak(b1, b1 + 40) > 0.02f) b1++;
    int waves = 0;                                      /* its length: the waves it has (the filters ring on after) */
    for (size_t i = b0 + 1; i < b1; i++) if (pcm[i - 1] < 0 && pcm[i] >= 0 && SDL_fabsf(pcm[i] - pcm[i - 1]) > 0.02f) waves++;
    double bp = pitch(b0 + 50, b0 + 600), blen = waves / bp;
    SDL_Log("beep_low: %.1f Hz, %.3f s", bp, blen);
    CHECK(SDL_fabs(bp - 1193182.0 / 500) < 60 && SDL_fabs(blen - 0.020) < 0.004, "beep %.1f Hz %.3f s", bp, blen);
    key(m, PK_KP_MINUS);

    /* go to war: the scroll unrolls, crackling */
    wav_start();
    key(m, PK_ENTER);
    run_menu(m, 6);
    wav_save("scroll_unroll");
    CHECK(peak(0, npcm) > 0.05f, "no crackle");
    key(m, PK_ESC);
    run_menu(m, 4);

    /* the battle report: won, then lost */
    static Play p;
    static Campaign c;
    c = camps[0];
    play_start(&p, &c, &ga, 7);
    spk_silence(SND_PLAY);                              /* its new game's notes: not over the report (main.c to_menu) */
    p.result_code = 0;
    wav_start();
    menu_report(m, &p);
    run_menu(m, 14);
    wav_save("report_won_tune");
    CHECK(peak(0, npcm) > 0.05f, "no victory tune");
    key(m, PK_SPACE);
    run_menu(m, 3);
    p.result_code = 11;
    wav_start();
    menu_report(m, &p);
    run_menu(m, 14);
    wav_save("report_lost_tune");
    CHECK(peak(0, npcm) > 0.05f, "no defeat tune");
    key(m, PK_SPACE);
    run_menu(m, 3);

    /* the opening: make changes, the campaign menu's fifth item */
    wav_start();
    key(m, PK_KP_PLUS);
    key(m, PK_ENTER);
    run_menu(m, 3);
    for (int i = 0; i < 4; i++) { key(m, PK_KP_PLUS); run_menu(m, 1.5); }
    key(m, PK_ENTER);
    wav_start();
    run_menu(m, 7);
    wav_save("opening_tune");
    CHECK(peak(0, npcm) > 0.05f, "no opening tune");
    menu_destroy(m);

    /* the map: a new game's three low notes, a key's beep, the buzz, a message */
    spk_silence(SND_MENU);
    render(0.2);
    wav_start();
    c = camps[0];
    play_start(&p, &c, &ga, 7);
    for (int i = 0; i < 27 * 10; i++) { play_frame(&p, 0); render(1.0 / PLAY_HZ); }
    wav_save("new_game");
    size_t a = first_sound(0), z = npcm;
    while (z > 0 && SDL_fabsf(pcm[z - 1]) < 0.02f) z--;
    SDL_Log("a new game: sound from %.2f s to %.2f s, the first note %.1f Hz", (double)a / SOUND_RATE, (double)z / SOUND_RATE,
            pitch(a + 200, a + 4000));
    CHECK(SDL_fabs(pitch(a + 200, a + 4000) - 1193182.0 / 0x2328) < 6, "play_end_sound's first note");
    wav_start();
    play_key(&p, PK_T, false);
    for (int i = 0; i < 27; i++) { play_frame(&p, 0); render(1.0 / PLAY_HZ); }
    for (int i = 0; i < 27 * 3; i++) { play_frame(&p, PK_UP); render(1.0 / PLAY_HZ); }
    wav_save("map_keys");
    CHECK(peak(0, npcm) > 0.05f, "no sound on the map");

    /* a battle: Practice with F.D.Roosevelt (as test_battle), the guns fired */
    spk_silence(SND_PLAY);
    static Battle bt;
    c = camps[0];
    play_start(&p, &c, &ga, 77);
    for (int n = 0; n < 2000 && (p.cx != 3 || p.cy != 186); n++) {
        int k = 0;
        if (!play_busy(&p)) {
            int dx = (3 > p.cx) - (3 < p.cx), dy = (186 > p.cy) - (186 < p.cy);
            k = PK_HOME + (dy + 1) * 4 + (dx + 1);
        }
        play_frame(&p, k);
        render(1.0 / PLAY_HZ);
    }
    for (int n = 0; n < 200 && play_busy(&p); n++) { play_frame(&p, 0); render(1.0 / PLAY_HZ); }
    play_key(&p, PK_P, false);
    for (int n = 0; n < 50 && p.battle_req != 2; n++) { play_frame(&p, 0); render(1.0 / PLAY_HZ); }
    CHECK(p.battle_req == 2, "Practice asked for");
    wav_start();
    if (battle_start(&bt, &p, true)) {
        for (int t = 0; t < 4000; t += 16) { battle_run(&bt, 16000); render(0.016); }
        battle_key(&bt, 'f', 0);
        for (int t = 0; t < 4000; t += 16) { battle_run(&bt, 16000); render(0.016); }
        battle_key(&bt, 'l', 0);
        for (int t = 0; t < 1500 && bt.state == BATTLE_RUNNING; t += 16) { battle_run(&bt, 16000); render(0.016); }
        battle_finish(&bt);
        render(0.5);
    }
    wav_save("battle_practice");
    SDL_Log("battle: %d sounds", bt.beeps);
    CHECK(bt.beeps > 0 && peak(0, npcm) > 0.05f, "no sound in the battle");

    /* the sounds off: nothing at all; "do not play music": beeps, no tunes */
    set_sound(0);
    m = menu_create(&ga, camps, data);
    set_sound(0);
    wav_start();
    menu_start(m);
    run_menu(m, 4);
    key(m, PK_KP_PLUS);
    key(m, PK_ENTER);
    run_menu(m, 3);
    wav_save("sounds_off");
    CHECK(peak(0, npcm) < 0.01f, "sound with the sounds off (%.3f)", peak(0, npcm));
    menu_destroy(m);
    set_sound(1);
    m = menu_create(&ga, camps, data);
    set_sound(1);
    wav_start();
    menu_start(m);
    run_menu(m, 4);
    CHECK(peak(0, npcm) < 0.01f, "a tune with no music");
    key(m, PK_KP_PLUS);
    CHECK(peak(0, npcm) > 0.05f, "no beep with no music");
    wav_save("no_music");
    menu_destroy(m);

    SDL_free(pcm);
    sound_quit();
    SDL_Log("%s", fails ? "FAILED" : "all passed");
    return fails != 0;
}
