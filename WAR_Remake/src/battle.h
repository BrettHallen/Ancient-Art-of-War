/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * battle.h: step 4, the battle screen (Zoom; WAR.EXE overlay 7).
 *
 * The soldiers are animated by the original's own scripts in O6 and O7: each
 * step moves a soldier in the off-screen buffer (dy * 80 + dx), shows one of
 * the soldier pictures and sets his state. battle_soldiers (ovl7:5258)
 * chooses the next script for each soldier from his orders, the terrain and
 * the nearest enemy. Units: x is a byte column (4 pixels), y a line pair.
 */
#ifndef WAR_BATTLE_H
#define WAR_BATTLE_H

#include "play.h"

#define BATTLE_HZ   8       /* frames a second (about an IBM PC/XT) */
#define MAX_MEN     28
#define MAX_ARROWS  10
#define BMEM_O7     0x2000  /* O7's place in Battle.mem (O6 is at 0) */

typedef struct { int sprite, addr, side; } BattleDraw;

typedef struct {
    Play    *p;
    uint8_t  mem[0x2800];     /* copies of O6 and O7: scripts are changed during a battle */
    bool     ok;
    int      me, en;          /* A50E, A50C: the two squads */
    int      bg;              /* A5CA: background 0-13 (P0-PD) */
    int      skill_me, skill_en;   /* AB89, AB8B */
    int      ymin[80], ymax[80];   /* A9A5, A9A7: where soldiers may stand */
    /* soldiers */
    int type[MAX_MEN], side[MAX_MEN], addr[MAX_MEN], x[MAX_MEN], y[MAX_MEN];
    int state[MAX_MEN], script[MAX_MEN], tgt[MAX_MEN], stat[MAX_MEN], shot[MAX_MEN];
    /* arrows */
    int ax[MAX_ARROWS], ay[MAX_ARROWS], aaddr[MAX_ARROWS], akind[MAX_ARROWS], apow[MAX_ARROWS], aside[MAX_ARROWS];
    int target;               /* A8C7 */
    int start;                /* A8CD: 20 waiting for the attack, then 7..0 */
    int endtimer;             /* A8C9 */
    int retreat;              /* A8CF: 1 you retreat, 2 the enemy, 3 you are routed */
    int menu;                 /* A8D1: 1 choose men, 2 choose order */
    bool asking;              /* A8CB: "Attack ... Retreat" */
    int  hold;                /* frames to wait while a bugle call sounds */
    int group, group_col;     /* AB95, AB97 */
    int order[3], order_time[3];   /* AB77, AB7D: knights, barbarians, archers */
    bool alive_type[3];       /* A8C1, A8C3, A8C5: your men of each type left */
    int lost_me, lost_en;     /* A8D7, AB93 */
    bool done;                /* A8D3 */
    int key;                  /* A8D5 */
    char text[2][41];         /* rows 23 and 24 */
    int trow, tcol;
    BattleDraw draw[MAX_MEN + MAX_ARROWS];
    int ndraw;
    int frame;                /* the remake: frames since the battle started (for the log) */
} Battle;

/* Set up the battle for squad sel (from Zoom). False if the files are missing. */
bool battle_start(Battle *b, Play *p, int sel);
void battle_key(Battle *b, int key);
void battle_frame(Battle *b);
void battle_draw(const Battle *b, Screen *s, GfxMode mode);
/* After done: apply the results and go back to the map. */
void battle_finish(Battle *b);

#endif
