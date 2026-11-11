/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * sound.c: step 8, the PC speaker and the Tandy/PCjr sound chip, played
 * through SDL3 audio. The routines are queued as segments, each holding
 * the speaker's timer divisor and the three Tandy voices for so many
 * samples; the audio callback turns them into square waves.
 */
#include "sound.h"

#include <SDL3/SDL.h>

#define RATE       44100
#define PIT_HZ     1193182.0
#define SN_HZ      (3579545.0 / 32.0)     /* SN76496 tone: clock / (32 x N) */
#define MAX_SEGS   4096
#define PC_LEVEL   0.22f
#define SN_LEVEL   0.16f

typedef struct {
    uint32_t samples;
    uint16_t pc_div;          /* timer 2 divisor; 0: the speaker is off */
    uint16_t sn[3];           /* Tandy tone registers N */
    uint8_t  att[3];          /* Tandy attenuation, 15 = off */
} Seg;

static struct {
    SDL_AudioStream *stream;
    SDL_Mutex *lock;
    Seg     q[MAX_SEGS];
    int     head, count;      /* ring buffer */
    uint32_t done;            /* samples played of q[head] */
    int     setting;
    double  pc_phase, sn_phase[3];
    float   hp_x, hp_y;       /* the speaker's high-pass (no DC) */
    /* state kept between queued segments */
    uint16_t sn[3];
    uint8_t  att[3];
    bool     scroll_on;
    bool     scrolling;
} S = { .setting = SOUND_ON, .att = { 15, 15, 15 } };

/* ------------------------------------------------------------------ generator */

static float poly_blep(double t, double dt)   /* band-limits the square's edges */
{
    if (t < dt) { t /= dt; return (float)(t + t - t * t - 1.0); }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float)(t * t + t + t + 1.0); }
    return 0.0f;
}

static float square(double *phase, double hz)
{
    if (hz <= 0.0 || hz > RATE / 2.0) return 0.0f;   /* WAR.EXE's divisor 17 (70 kHz) is silence */
    double dt = hz / RATE;
    double t = *phase;
    float v = t < 0.5 ? 1.0f : -1.0f;
    v += poly_blep(t, dt);
    double t2 = t + 0.5; if (t2 >= 1.0) t2 -= 1.0;
    v -= poly_blep(t2, dt);
    *phase = t + dt; if (*phase >= 1.0) *phase -= 1.0;
    return v;
}

static float sn_level(int att) { return att >= 15 ? 0.0f : SN_LEVEL * SDL_powf(10.0f, -(float)att / 10.0f); }

void sound_render(float *buf, int n)
{
    if (!S.lock) { SDL_memset(buf, 0, n * sizeof *buf); return; }
    SDL_LockMutex(S.lock);
    for (int i = 0; i < n; i++) {
        float v = 0.0f;
        if (S.count) {
            const Seg *g = &S.q[S.head];
            if (g->pc_div) v += PC_LEVEL * square(&S.pc_phase, PIT_HZ / g->pc_div);
            for (int c = 0; c < 3; c++)
                if (g->att[c] < 15 && g->sn[c]) v += sn_level(g->att[c]) * square(&S.sn_phase[c], SN_HZ / g->sn[c]);
            if (++S.done >= g->samples) {
                S.done = 0;
                S.head = (S.head + 1) % MAX_SEGS;
                S.count--;
            }
        }
        /* a one-pole high-pass at about 20 Hz, like the speaker cone */
        float y = 0.997f * (S.hp_y + v - S.hp_x);
        S.hp_x = v;
        S.hp_y = y;
        buf[i] = y;
    }
    SDL_UnlockMutex(S.lock);
}

static void SDLCALL callback(void *ud, SDL_AudioStream *stream, int additional, int total)
{
    (void)ud; (void)total;
    float buf[1024];
    int want = additional / (int)sizeof(float);
    while (want > 0) {
        int n = SDL_min(want, (int)SDL_arraysize(buf));
        sound_render(buf, n);
        SDL_PutAudioStreamData(stream, buf, n * (int)sizeof(float));
        want -= n;
    }
}

/* ------------------------------------------------------------------ queue */

static double unit_us = SOUND_UNIT_US;
void sound_unit(double us) { unit_us = us > 0.0 ? us : SOUND_UNIT_US; }

static uint32_t units_to_samples(double units) { return (uint32_t)(units * unit_us * 1e-6 * RATE + 0.5); }

static void push(uint32_t samples, int pc_div)
{
    if (!S.lock || samples == 0) return;
    SDL_LockMutex(S.lock);
    if (S.count < MAX_SEGS) {
        Seg *g = &S.q[(S.head + S.count) % MAX_SEGS];
        g->samples = samples;
        g->pc_div = (uint16_t)pc_div;
        for (int c = 0; c < 3; c++) { g->sn[c] = S.sn[c]; g->att[c] = S.att[c]; }
        S.count++;
    }
    SDL_UnlockMutex(S.lock);
}

bool sound_init_offline(void)
{
    if (!S.lock) S.lock = SDL_CreateMutex();
    return S.lock != NULL;
}

bool sound_init(void)
{
    if (S.stream) return true;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) { SDL_Log("No audio: %s", SDL_GetError()); return false; }
    if (!S.lock) S.lock = SDL_CreateMutex();
    SDL_AudioSpec spec = { SDL_AUDIO_F32, 1, RATE };
    S.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, callback, NULL);
    if (!S.stream) {                    /* no device: nothing is queued */
        SDL_Log("No audio device: %s", SDL_GetError());
        SDL_DestroyMutex(S.lock);
        S.lock = NULL;
        return false;
    }
    SDL_ResumeAudioStreamDevice(S.stream);
    return true;
}

void sound_quit(void)
{
    if (S.stream) SDL_DestroyAudioStream(S.stream);
    S.stream = NULL;
    if (S.lock) SDL_DestroyMutex(S.lock);
    S.lock = NULL;
}

void sound_setting(int s) { S.setting = SDL_clamp(s, SOUND_OFF, SOUND_TANDY); }
int  sound_get_setting(void) { return S.setting; }

void sound_stop(void)
{
    if (!S.lock) return;
    SDL_LockMutex(S.lock);
    S.count = 0;
    S.done = 0;
    SDL_UnlockMutex(S.lock);
    for (int c = 0; c < 3; c++) S.att[c] = 15;
}

double sound_busy(void)
{
    if (!S.lock) return 0.0;
    SDL_LockMutex(S.lock);
    double n = 0;
    for (int k = 0; k < S.count; k++) n += S.q[(S.head + k) % MAX_SEGS].samples;
    n -= S.done;
    SDL_UnlockMutex(S.lock);
    return n / RATE;
}

/* ------------------------------------------------------------------ the PC speaker */

void snd_tone(int divisor, int units)
{
    if (S.setting == SOUND_OFF) return;      /* speaker_on (rt:20EF) checks the setting */
    push(units_to_samples(units), divisor);
}

void snd_pause(int units) { push(units_to_samples(units), 0); }

void snd_beep(void)       { snd_tone(500, 400); }
void snd_beep_high(void)  { snd_tone(2000, 300); }
void snd_buzz(void)       { snd_tone(8000, 600); }
void snd_alert(void)      { snd_tone(1000, 1350); }
void snd_error(void)      { snd_tone(5000, 3000); }
void snd_kill(void)       { snd_tone(13000, 500); }
void snd_arrow(void)      { snd_tone(1190, 100); }

void snd_end_of_turn(void)
{
    if (S.setting <= SOUND_NO_MUSIC) return;
    snd_tone(9000, 5000);
    snd_tone(8500, 2500);
}

/* campaign_play (ovl6:317D, 3194, 31FE): a new game opens with play_end_sound,
 * the enemy's first five ai_ticks (about 0.8 s on an IBM PC/XT, measured in
 * DOSBox at 300 cycles), then play_end_sound twice more. */
void snd_new_game(void)
{
    if (S.setting <= SOUND_NO_MUSIC) return;
    snd_end_of_turn();
    push((uint32_t)(0.8 * RATE), 0);
    snd_end_of_turn();
    snd_end_of_turn();
}

void snd_group_full(void)
{
    snd_tone(5000, 1200);
    snd_pause(9000);
}

/* ------------------------------------------------------------------ tunes */

static int word(const uint8_t *b, unsigned off) { return b[off] | (b[off + 1] << 8); }

static void sn_set(int c, int n, int att) { S.sn[c] = (uint16_t)SDL_clamp(n, 0, 1023); S.att[c] = (uint8_t)att; }

void snd_tune(const uint8_t *buf, unsigned freqs, unsigned durs,
              unsigned v0, unsigned v1, unsigned v2, unsigned tdurs)
{
    if (S.setting == SOUND_ON) {             /* ovl1_462c: one voice on the speaker */
        for (int i = 0; word(buf, freqs + 2 * i); i++)
            push(units_to_samples(word(buf, durs + 2 * i) * 4), word(buf, freqs + 2 * i));
    } else if (S.setting == SOUND_TANDY) {   /* ovl1_469f: three voices, then a fade */
        for (int i = 0; word(buf, v0 + 2 * i); i++) {
            sn_set(0, word(buf, v0 + 2 * i) / 15, 0);
            sn_set(1, word(buf, v1 + 2 * i) / 15, 2);
            sn_set(2, word(buf, v2 + 2 * i) / 15, 1);
            push(units_to_samples(word(buf, tdurs + 2 * i) * 4), 0);
        }
        for (int a = 2; a < 13; a++) {
            S.att[0] = (uint8_t)a; S.att[1] = (uint8_t)(a + 2); S.att[2] = (uint8_t)(a + 1);
            push(units_to_samples(3000), 0);
        }
        for (int c = 0; c < 3; c++) S.att[c] = 15;   /* rt:2171 */
    }
}

void snd_bugle(const uint8_t *buf, unsigned off)
{
    if (S.setting == SOUND_ON) {             /* ovl7:6AF7: each call, then a rest */
        for (unsigned p = off; word(buf, p); p += 4) {
            push(units_to_samples(word(buf, p + 2) * 4), word(buf, p));
            push(units_to_samples(500), 0);
        }
    } else if (S.setting == SOUND_TANDY) {   /* ovl7:6B63: a chord on the Tandy chip */
        S.att[2] = 7;
        for (unsigned p = off; word(buf, p); p += 4) {
            int n = word(buf, p) / 15;
            S.att[0] = 0;
            S.att[1] = 1;
            S.sn[0] = (uint16_t)n;
            S.sn[1] = (uint16_t)(n + n / 4);
            S.sn[2] = (uint16_t)(n + 2 * n / 3);
            push(units_to_samples(word(buf, p + 2) * 4), 0);
            S.att[0] = 15; S.att[2] = 15; S.att[1] = 14;   /* the second voice goes on very softly */
            push(units_to_samples(500), 0);
        }
        for (int c = 0; c < 3; c++) S.att[c] = 15;
    }
}

/* ------------------------------------------------------------------ the scroll */

void snd_scroll_step(bool toggle, double seconds)
{
    if (S.setting == SOUND_OFF) return;
    if (!S.scrolling) { S.scrolling = true; S.scroll_on = true; }   /* speaker_on: gate and data on */
    if (toggle) S.scroll_on = !S.scroll_on;                          /* rt:20CE flips the data bit */
    push((uint32_t)(seconds * RATE + 0.5), S.scroll_on ? 30000 : 0);
}

void snd_scroll_end(void) { S.scrolling = false; }
