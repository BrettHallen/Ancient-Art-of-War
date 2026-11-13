/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * sound.h: step 8, the PC speaker.
 *
 * WAS.EXE has only the PC speaker (no Tandy chip): speaker_on (rt:2F59)
 * sets port 61h's bits 0 and 1 (timer 2's gate and the speaker's data),
 * rt_2f3f / speaker_freq (rt:2F46) gives timer 2 a divisor of 1193182 Hz,
 * rt_2f67 clears both bits, speaker_click (rt:2F31) flips the data bit
 * (a crackle, or a click with the timer off). speaker_on, rt_2f3f and
 * speaker_click do nothing with the sounds off ([684F] = 0); the tunes play
 * only with "all sounds are on" ([684F] = 2). How long a sound lasts is the
 * delay() between these calls, so each is played at the time of the clock
 * of the part of the remake that made it (overlay 1's thread, the battle's
 * thread, campaign play's iterations): 50 us a delay unit, an IBM PC/XT's.
 */
#ifndef WAS_SOUND_H
#define WAS_SOUND_H

#include <stdbool.h>
#include <stdint.h>

/* the parts of the remake that make sounds, each with its own clock */
typedef enum { SND_MENU, SND_BATTLE, SND_PLAY, SND_TEST, SND_SOURCES } SoundSource;

bool   sound_init(void);                 /* false: no audio device (the game runs silent) */
void   sound_quit(void);
void   sound_options(const uint8_t *scnf);   /* SCNF (DS:684C ...): [684F] read as it changes */
int    sound_setting(void);              /* [684F]: 0 off, 1 no music, 2 all on */

/* port 61h and timer 2, at time t_us of the source's clock */
void   spk_on(SoundSource src, double t_us);              /* speaker_on rt:2F59 */
void   spk_freq(SoundSource src, double t_us, int divisor);   /* rt_2f3f */
void   spk_off(SoundSource src, double t_us);             /* rt_2f67 */
void   spk_click(SoundSource src, double t_us);           /* speaker_click rt:2F31 */
void   spk_silence(SoundSource src);                      /* the source stopped: the speaker off now */

/* the tests: no device; the events are rendered on demand (mono 44100 Hz) */
bool   sound_init_offline(void);
void   sound_render(float *buf, int n);
int    sound_events(void);               /* events still to play */

#define SOUND_RATE 44100

#endif
