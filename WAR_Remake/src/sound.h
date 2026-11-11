/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * sound.h: step 8, the PC speaker and the Tandy/PCjr sound chip.
 *
 * WAR.EXE drives the PC speaker through timer 2 (a divisor of 1193182 Hz)
 * and the Tandy 1000/PCjr SN76496 at port C0h (three tone voices,
 * attenuation in 2 dB steps). Every sound is a series of these settings,
 * each held for a number of units of its delay() routine: n x m calls of
 * random(), with m from a calibration loop timed against the timer chip.
 * On an IBM PC/XT (4.77 MHz 8088) the loop count is about 1200, so m is 1,
 * and a call of random() with its loop takes about 240 clocks: 50
 * microseconds a unit (the opening tune lasts 5 s). DOSBox runs the
 * calibration loop relatively faster and plays every sound about 2.6 times
 * too fast (19 microseconds, at 300 or 1500 cycles alike). Here the
 * sounds are queued and played by SDL; WAR.EXE
 * waited for each one, so callers that should pause (the battle's bugle
 * calls, the tunes) ask how long the queue still plays.
 */
#ifndef WAR_SOUND_H
#define WAR_SOUND_H

#include <stdbool.h>
#include <stdint.h>

#define SOUND_UNIT_US 50.0     /* one unit of WAR.EXE's delay() on an IBM PC/XT */

/* The Options Shoppe setting (DS:A6E1) */
enum { SOUND_OFF = 0, SOUND_NO_MUSIC = 1, SOUND_ON = 2, SOUND_TANDY = 3 };

bool   sound_init(void);       /* false: no audio device (the game runs silent) */
void   sound_quit(void);
bool   sound_init_offline(void);   /* tests: queue without a device, render with sound_render */
void   sound_render(float *buf, int n);   /* the next n samples (mono, 44100 Hz) */
void   sound_unit(double us);         /* microseconds per delay unit (tests) */
void   sound_setting(int s);   /* SOUND_OFF ... SOUND_TANDY */
int    sound_get_setting(void);
void   sound_stop(void);       /* drop everything queued */
double sound_busy(void);       /* seconds of sound still queued */

/* The PC speaker: a tone of timer-2 divisor d for n delay units (speaker_on,
 * speaker_freq, delay, speaker_off), and a pause of n units. */
void snd_tone(int divisor, int units);
void snd_pause(int units);

/* WAR.EXE's sound routines */
void snd_beep(void);           /* rt:05A9 beep_low: 2386 Hz, 400 units (most keys) */
void snd_beep_high(void);      /* rt:05CC beep_high: 597 Hz, 300 units */
void snd_buzz(void);           /* rt:05E9: 149 Hz, 600 units (the cursor over a squad) */
void snd_alert(void);          /* rt:22F5: 1193 Hz, 1350 units (messages, battle orders) */
void snd_error(void);          /* rt:230C: 239 Hz, 3000 units */
void snd_end_of_turn(void);    /* play_end_sound ovl6:38F6: 133 Hz then 140 Hz (sound on only) */
void snd_new_game(void);       /* ovl6:317D-31FE: one of those, a pause while the enemy plans, two more */
void snd_group_full(void);     /* ovl6:69BF: 239 Hz, 1200 units, then a pause of 9000 */
void snd_kill(void);           /* ovl7:6511: 92 Hz, 500 units (a man falls) */
void snd_arrow(void);          /* ovl7:7ADD: 1003 Hz, 100 units (an arrow is loosed) */

/* The tunes (O2): ovl1_462c on the PC speaker or ovl1_469f on the Tandy
 * chip, by the setting; nothing with sounds off or "do not play music".
 * Offsets of zero-terminated word tables in buf. */
void snd_tune(const uint8_t *buf, unsigned freqs, unsigned durs,
              unsigned v0, unsigned v1, unsigned v2, unsigned tdurs);

/* The battle's bugle calls (ovl7_6ae9; O7 pairs of divisor, length). */
void snd_bugle(const uint8_t *buf, unsigned off);

/* The scroll unrolling (ovl1:75C5): timer 2 at 40 Hz, the speaker switched
 * on or off at random each step (rt:20CE) */
void snd_scroll_step(bool toggle, double seconds);
void snd_scroll_end(void);

#endif
