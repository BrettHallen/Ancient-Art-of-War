/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * gamelog.h: the debug log (--log, --log=verbose). Each campaign started (or
 * saved game resumed) gets its own text file, named with the date and time,
 * in the remake's folder under Logs/. It records the campaign and its
 * settings, then everything the enemy commander decides and why, what you
 * order, what happens on the map and on the battle screen.
 *
 * Normal: decisions, orders and events. Verbose: also each squad the enemy
 * looks at, route searches, candidate scores, the cell values, and every 16
 * ticks a table of all squads.
 *
 * Logging never calls random() or changes the game: a logged game plays the
 * same as one not logged (test/test_gamelog.c).
 */
#ifndef WAR_GAMELOG_H
#define WAR_GAMELOG_H

#include "play.h"

#include <SDL3/SDL.h>

typedef enum { GLOG_OFF = 0, GLOG_NORMAL = 1, GLOG_VERBOSE = 2 } GameLogLevel;

/* main: the level and the folder for the files (created if missing) */
void gamelog_setup(GameLogLevel level, const char *dir);
GameLogLevel gamelog_level(void);

/* A new log for the game in p (any open one is closed first). how: "New
 * campaign", "Saved game resumed"... Called before play_start fills p (the
 * header comes from the campaign), or after a saved game is loaded. */
void gamelog_begin(const Play *p, const Campaign *c, const char *how, uint32_t seed, int fight_delay);
/* The state of the game: opponent's values, flags, squads, the enemy's plans
 * (play.c calls it once the game is set up; resumed games straight away) */
void gamelog_state(const Play *p, const char *title);
void gamelog_squads(const Play *p, const char *title);                  /* just the squads */
void gamelog_grid(const Play *p, const char *title, const int16_t *v);  /* verbose: a 10x20 cell table */
/* The game is over or left (the log is closed) */
void gamelog_end(const Play *p, const char *how);
const char *gamelog_path(void);   /* the open file, or "" */

/* is p the game being logged, at this level? */
bool gamelog_on(const Play *p, GameLogLevel level);
/* one line, after the game time: [mm:ss tick N] */
void gamelog_printf(const Play *p, const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(2);
/* the clock: each main-loop iteration, and each tick of game time */
void gamelog_iteration(const Play *p);
void gamelog_tick(const Play *p);

/* words for the log (each call returns its own buffer, a few at a time) */
const char *gamelog_squad(const Play *p, int i);    /* "your squad 3 (4 archers, 2 knights) at 38,145" */
const char *gamelog_cell(const Play *p, int cell);  /* "cell 143 (row 14, col 3, fort)" */
const char *gamelog_key(int key);                   /* "M", "Enter"... */

#define GLOG(p, level, ...) do { if (gamelog_on((p), (level))) gamelog_printf((p), __VA_ARGS__); } while (0)

#endif
