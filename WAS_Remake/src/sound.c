/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * sound.c: step 8, the PC speaker played through SDL3 audio.
 *
 * Each change to port 61h or timer 2 is an event stamped with the time of
 * the clock of the part of the remake that made it. A source's clock is
 * tied to the audio's by an offset, set when its first event comes (or when
 * one would be too late, or too far ahead, after the source stopped or
 * started again), so that its sounds play a little ahead of the samples
 * already made, spaced as the source's delays spaced them. The audio
 * callback applies the events at their samples and makes the speaker's
 * wave: timer 2's square when gate and data are on, the cone held out when
 * only data is on, nothing otherwise; a high-pass takes the steady part
 * away as the speaker's cone does, so a flip of the data bit with the timer
 * off is a click. (The square is taken about its middle: a real speaker
 * pops as a note starts, which is left out.)
 */
#include "sound.h"

#include <SDL3/SDL.h>

#define PIT_HZ     1193182.0
#define MAX_EVS    16384
#define LEVEL      0.2f
#define LEAD_S     0.08                  /* how far ahead of the samples made a source's sounds are put */
#define AHEAD_S    10.0                  /* further ahead than this: the source's clock jumped */

enum { E_ON, E_OFF, E_FREQ, E_CLICK };

typedef struct {
    uint64_t at;                         /* the sample */
    uint8_t  kind;
    uint8_t  src;
    uint16_t div;
} Ev;

static struct {
    SDL_AudioStream *stream;
    SDL_Mutex *lock;
    bool     offline;
    Ev       q[MAX_EVS];
    int      head, count;
    uint64_t made;                       /* samples made so far */
    double   off_us[SND_SOURCES];        /* source time + off_us = audio time */
    bool     synced[SND_SOURCES];
    double   last_us[SND_SOURCES];
    double   last_at[SND_SOURCES];
    /* the speaker as the callback has it */
    bool     gate, data;
    uint16_t div;
    double   phase;
    float    lp, hp_x, hp_y;
    const uint8_t *scnf;
} S = { .div = 0x10000 - 1 };

int sound_setting(void) { return S.scnf ? (int16_t)(S.scnf[3] | (S.scnf[4] << 8)) : 2; }   /* [684F] */
void sound_options(const uint8_t *scnf) { S.scnf = scnf; }

/* ------------------------------------------------------------------ the wave */

static float poly_blep(double t, double dt)          /* band-limits the square's edges */
{
    if (t < dt) { t /= dt; return (float)(t + t - t * t - 1.0); }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float)(t * t + t + t + 1.0); }
    return 0.0f;
}

static float speaker(void)
{
    if (!S.data) return 0.0f;
    if (!S.gate) return 1.0f;                        /* the timer's output stays high: the cone held out */
    double hz = PIT_HZ / (S.div ? S.div : 65536);
    if (hz > SOUND_RATE / 2.0) return 0.0f;          /* above hearing (WAS.EXE's divisor 17 is a rest) */
    double dt = hz / SOUND_RATE, t = S.phase;
    float v = t < 0.5 ? 1.0f : -1.0f;
    v += poly_blep(t, dt);
    double t2 = t + 0.5;
    if (t2 >= 1.0) t2 -= 1.0;
    v -= poly_blep(t2, dt);
    S.phase = t + dt;
    if (S.phase >= 1.0) S.phase -= 1.0;
    return v;                                        /* about its middle, so a note starts without a pop */
}

static void apply(const Ev *e)
{
    switch (e->kind) {
    case E_ON:    S.gate = S.data = true; break;
    case E_OFF:   S.gate = S.data = false; break;
    case E_FREQ:  S.div = e->div; break;
    default:      S.data = !S.data; break;
    }
}

void sound_render(float *buf, int n)
{
    if (!S.lock) { SDL_memset(buf, 0, (size_t)n * sizeof *buf); return; }
    SDL_LockMutex(S.lock);
    for (int i = 0; i < n; i++) {
        while (S.count && S.q[S.head].at <= S.made) {
            apply(&S.q[S.head]);
            S.head = (S.head + 1) % MAX_EVS;
            S.count--;
        }
        float v = speaker();
        S.lp += 0.55f * (v - S.lp);                  /* the cone can't follow the sharpest edges */
        float y = 0.997f * (S.hp_y + S.lp - S.hp_x); /* nor hold itself out: a high-pass at about 20 Hz */
        S.hp_x = S.lp;
        S.hp_y = y;
        buf[i] = LEVEL * y;
        S.made++;
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

/* ------------------------------------------------------------------ the events */

static void push(SoundSource src, double t_us, int kind, int div)
{
    if (!S.lock || src < 0 || src >= SND_SOURCES) return;
    SDL_LockMutex(S.lock);
    double lead = LEAD_S * SOUND_RATE, now = (double)S.made;
    double at = (t_us + S.off_us[src]) * (SOUND_RATE / 1e6);
    if (!S.synced[src] || t_us < S.last_us[src] || at > now + lead + AHEAD_S * SOUND_RATE) {
        at = now + lead;                             /* tie the source's clock to the audio's */
        S.synced[src] = true;
    } else if (at < now) {                           /* behind: later, but after its sounds already there */
        at = SDL_max(now + lead, S.last_at[src]);
    }
    S.off_us[src] = at * (1e6 / SOUND_RATE) - t_us;
    S.last_us[src] = t_us;
    S.last_at[src] = at;
    if (S.count < MAX_EVS) {
        Ev e = { (uint64_t)(at + 0.5), (uint8_t)kind, (uint8_t)src, (uint16_t)div };
        int i = S.count++;
        while (i > 0 && S.q[(S.head + i - 1) % MAX_EVS].at > e.at) {   /* in order of time (the sources mix) */
            S.q[(S.head + i) % MAX_EVS] = S.q[(S.head + i - 1) % MAX_EVS];
            i--;
        }
        S.q[(S.head + i) % MAX_EVS] = e;
    }
    SDL_UnlockMutex(S.lock);
}

void spk_on(SoundSource src, double t_us)
{
    if (sound_setting() != 0) push(src, t_us, E_ON, 0);
}

void spk_freq(SoundSource src, double t_us, int divisor)
{
    if (sound_setting() != 0) push(src, t_us, E_FREQ, divisor & 0xFFFF);
}

void spk_off(SoundSource src, double t_us) { push(src, t_us, E_OFF, 0); }

void spk_click(SoundSource src, double t_us)
{
    if (sound_setting() != 0) push(src, t_us, E_CLICK, 0);
}

void spk_silence(SoundSource src)
{
    if (!S.lock || src < 0 || src >= SND_SOURCES) return;
    SDL_LockMutex(S.lock);
    int n = 0;                                       /* its sounds still to come are dropped */
    for (int i = 0; i < S.count; i++) {
        Ev e = S.q[(S.head + i) % MAX_EVS];
        if (e.src != src) S.q[(S.head + n++) % MAX_EVS] = e;
    }
    S.count = n;
    S.synced[src] = false;
    SDL_UnlockMutex(S.lock);
    push(src, 0, E_OFF, 0);
    SDL_LockMutex(S.lock);
    S.synced[src] = false;
    SDL_UnlockMutex(S.lock);
}

int sound_events(void)
{
    if (!S.lock) return 0;
    SDL_LockMutex(S.lock);
    int n = S.count;
    SDL_UnlockMutex(S.lock);
    return n;
}

/* ------------------------------------------------------------------ the device */

bool sound_init_offline(void)
{
    if (!S.lock) S.lock = SDL_CreateMutex();
    S.offline = true;
    return S.lock != NULL;
}

bool sound_init(void)
{
    if (S.stream) return true;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) { SDL_Log("No audio: %s", SDL_GetError()); return false; }
    if (!S.lock) S.lock = SDL_CreateMutex();
    SDL_AudioSpec spec = { SDL_AUDIO_F32, 1, SOUND_RATE };
    S.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, callback, NULL);
    if (!S.stream) {                                 /* no device: nothing is queued */
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
