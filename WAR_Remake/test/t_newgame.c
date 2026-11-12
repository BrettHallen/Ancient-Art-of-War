/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/t_newgame.c: a test or debugging aid for the remake (see README.md here)
 */
#include "../src/sound.h"
#include <SDL3/SDL.h>
int main(void) { sound_init_offline(); sound_setting(SOUND_ON); snd_new_game(); SDL_Log("new game sound %.3f s", sound_busy());
  sound_stop(); sound_setting(SOUND_NO_MUSIC); snd_new_game(); SDL_Log("no music %.3f s", sound_busy()); return 0; }
