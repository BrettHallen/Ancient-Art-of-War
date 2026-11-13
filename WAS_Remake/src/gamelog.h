/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * gamelog.h: the debug log (--log, --log=verbose), as the WAR remake's.
 * Each campaign started (or saved game resumed) gets its own text file,
 * named with the date and time, in the remake's folder under Logs/. It
 * records the campaign and its settings, then everything the enemy
 * commander decides and why, what you order, what happens on the map and in
 * the sea battles and boardings.
 *
 * Normal: decisions, orders and events. Verbose: also each squadron the
 * enemy looks at and why it stays, every cell it weighed with its score, the
 * strength and port maps whenever they are worked out again, the ports'
 * supplies and taking on supplies and repairs, broadsides fired, and every
 * 16 ticks a table of all squadrons.
 *
 * Logging never calls random() and never touches the game or its timing
 * (the routines that count work for the XT's speed are not called): a logged
 * game plays the same as one not logged (test/test_gamelog.c).
 */
#ifndef WAS_GAMELOG_H
#define WAS_GAMELOG_H

#include "play.h"

#include <SDL3/SDL.h>

typedef enum { GLOG_OFF = 0, GLOG_NORMAL = 1, GLOG_VERBOSE = 2 } GameLogLevel;

/* main: the level and the folder for the files (created if missing) */
void gamelog_setup(GameLogLevel level, const char *dir);
GameLogLevel gamelog_level(void);

/* A new log for the game in p (any open one is closed first). how: "New
 * campaign", "Saved game resumed"... Called before play_start fills p (the
 * header comes from the campaign; play.c writes the game at the start), or
 * after a saved game is loaded. rnd, ptr: random()'s state (DS:0808, 080A). */
void gamelog_begin(const Play *p, const Campaign *c, const GameAssets *ga, const char *how, unsigned rnd, unsigned ptr);
/* The state of the game: the opponent's values, ports, squadrons, the
 * enemy's plans (verbose: the strength and port maps) */
void gamelog_state(const Play *p, const char *title);
void gamelog_squads(const Play *p, const char *title);   /* just the squadrons */
/* verbose: a 10x20 table of the cells */
void gamelog_grid(const Play *p, const char *title, const int16_t *v);
/* The game is over or left (the log is closed) */
void gamelog_end(const Play *p, const char *how);
const char *gamelog_path(void);   /* the open file, or "" */

/* is p the game being logged, at this level? (never the demo) */
bool gamelog_on(const Play *p, GameLogLevel level);
/* one line, after the game time: [mm:ss tick N] */
void gamelog_printf(const Play *p, const char *fmt, ...) SDL_PRINTF_VARARG_FUNC(2);
/* the clock: each main-loop iteration, and each tick of game time */
void gamelog_iteration(const Play *p);
void gamelog_tick(const Play *p);

/* words for the log (each call returns its own buffer, a few at a time) */
const char *gamelog_squad(const Play *p, int i);    /* "your squadron 3 (LEXINGTON flag, ...) at 38,145 cell 143 sea" */
const char *gamelog_ships(const Play *p, int i);    /* "LEXINGTON flag, SARATOGA line s2h1" */
const char *gamelog_ship(const Play *p, int s);     /* "LEXINGTON (flag)" */
const char *gamelog_cell(const Play *p, int cell);  /* "cell 143 (row 14 col 3, supply port)" */
const char *gamelog_dirs(const int8_t *steps, int n);   /* route directions 1-8: "E SE S ..." */
const char *gamelog_key(int key);                   /* "M", "Enter"... */
const char *gamelog_result(int code);               /* "You won: the enemy has no flagships left" */
int  gamelog_cell_of(const Play *p, int i);         /* the squadron's cell, as squadron_cell (rt:1985) */
int  gamelog_strength(const Play *p, int i);        /* as squadron_strength (ovl4:E8C8) */
int  gamelog_worst(const Play *p, int i);           /* its worst damage (ovl4:EE4E) */

#define GLOG(p, level, ...) do { if (gamelog_on((p), (level))) gamelog_printf((p), __VA_ARGS__); } while (0)

#endif
