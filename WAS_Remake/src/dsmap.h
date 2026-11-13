/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * dsmap.h: campaign play's state as WAS.EXE's data segment (see dsmap.c)
 */
#ifndef WAS_DSMAP_H
#define WAS_DSMAP_H

#include "play.h"

/* Write the state into ds (a 64 KB image of WAS.EXE's data segment; the
 * rest of the image is left as it is). */
void play_to_ds(const Play *p, uint8_t *ds);

/* Read back the parts of the state WAS.EXE's routines change. */
void play_from_ds(Play *p, const uint8_t *ds);

/* The saved game SS (DS:3F39-6804): everything save_game writes */
void play_save_ds(const Play *p, uint8_t *ds);
/* A Play and its campaign from SS's data segment (load_saved_game); the
 * random state goes on from the menus. False if the campaign can't be made. */
bool play_load_ds(Play *p, Campaign *c, const GameAssets *ga, const uint8_t *ds, uint16_t rnd, uint16_t rnd_ptr);

#endif
