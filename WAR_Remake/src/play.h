/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * play.h: step 3, the strategic game (campaign play, WAR.EXE overlay 6).
 *
 * Units follow WAR.EXE so the logic can be compared with the disassembly:
 * x is a byte column of the CGA map buffer (4 pixels), y a line pair
 * (2 pixels). The map is 80 x 200 of these (320 x 400 pixels).
 */
#ifndef WAR_PLAY_H
#define WAR_PLAY_H

#include "assets.h"
#include "campaign.h"
#include "gfx.h"

#define PLAY_HZ        20      /* main-loop iterations per second (about an IBM PC/XT) */
#define PATH_SIZE      1000    /* shared path memory (DS:9782) */
#define PATH_END       99      /* end of a path */
#define VIEW_TOP_MAX   0x6D    /* bottom of the map, in line pairs */
#define FIGHT_ON       (-1)    /* PSquad.clock: fighting on the map */

/* WAR.EXE's original scan codes are used as key codes. */
enum {
    PK_NONE = 0, PK_ESC = 0x01, PK_BACKSPACE = 0x0E, PK_R = 0x13, PK_ENTER = 0x1C, PK_A = 0x1E,
    PK_K = 0x25, PK_B = 0x30, PK_SPACE = 0x39, PK_MINUS = 0x0C, PK_EQUALS = 0x0D, PK_E = 0x12, PK_T = 0x14, PK_Y = 0x15,
    PK_I = 0x17, PK_S = 0x1F, PK_D = 0x20, PK_F = 0x21, PK_J = 0x24, PK_Z = 0x2C, PK_C = 0x2E,
    PK_V = 0x2F, PK_N = 0x31, PK_M = 0x32, PK_HOME = 0x47, PK_UP = 0x48, PK_PGUP = 0x49,
    PK_KP_MINUS = 0x4A, PK_LEFT = 0x4B, PK_KP_5 = 0x4C, PK_RIGHT = 0x4D, PK_KP_PLUS = 0x4E,
    PK_END = 0x4F, PK_DOWN = 0x50, PK_PGDN = 0x51,
    PK_Q = 0x10, PK_L = 0x26, PK_TAB = 0x0F, PK_CTRL = 0x1D,
    PK_W = 0x11, PK_U = 0x16, PK_O = 0x18, PK_P = 0x19, PK_G = 0x22, PK_H = 0x23, PK_X = 0x2D,
    PK_ALT_S = 0x11F,          /* Alt+S: save the game (ovl6:3BDA) */
};

typedef enum {               /* DS:A92F */
    PM_NORMAL = 0,           /* cursor is a cross */
    PM_SQUAD  = 1,           /* cursor over a squad: its commands are shown */
    PM_TRACE  = 2,           /* tracing a path: cursor is an open box, time stops */
    PM_SIZE   = 3,           /* Size: the men in the squad */
    PM_INFO   = 4,           /* Info: orders, condition and food */
    PM_GROUP  = 5,           /* Detach or Join: moving men between two squads */
    PM_FORMATION = 50,       /* Formation: choosing the squad's formation */
    PM_VIEW   = 98,          /* View: who holds which part of the map (remake's own number) */
    PM_PROMPT = 99,          /* a question, or the game is over: time stops */
} PlayMode;

typedef enum { PLAY_RUNNING, PLAY_WON, PLAY_LOST, PLAY_QUIT, PLAY_SAVE } PlayResult;

typedef struct {
    bool used;               /* 9EB0 != 9999 */
    bool friendly;           /* 9E60 = -1 */
    int  speed;              /* 9E88: the order, 1 fast, 2 march, 3 slow */
    int  moving;             /* 9EB0: 0 = stopped, else the speed it is moving at */
    int  x, y;               /* 9ED8, 9F00 */
    int  formation, condition, food;
    int  archers, knights, barbarians, spies;
    int  men;                /* 9FF0: archers + knights + barbarians */
    int  countdown;          /* A0CC: ticks until the next step */
    int  delay;              /* A8D9: ticks per step on this terrain */
    bool has_path;           /* A194 */
    int  path_pos;           /* A144: next step in the path memory */
    bool marked;             /* A51E != 9999: destination mark shown */
    int  mark_x, mark_y;
    bool fighting;           /* A0F4: in a battle with squad foe */
    int  foe;
    int  clock;              /* A004: 1..fight_delay counting down to the fight
                                (Zoom possible), FIGHT_ON once fighting, 0 none */
    int  kill;               /* A1E4: chance of killing a man each iteration */
    bool seen;               /* A11C: in sight of the other side */
    bool small;              /* A090 = 2: spies only, drawn as the small figure */
    /* the enemy commander's plans for this squad */
    int  on_route;           /* A1BC: following a route of map cells (nonzero) */
    int  route_pos;          /* A16C: its next step in the route memory */
    bool planned;            /* A0B8: has a destination */
    int  dest;               /* A02C: the destination cell */
} PSquad;

typedef struct {
    bool present, white;
    int  x, y;
} PFlag;

typedef struct {
    const Campaign  *c;
    const GameAssets *ga;
    PSquad   sq[MAX_SQUADS];
    PFlag    flag[MAX_FLAGS];
    int8_t   path[PATH_SIZE];
    int      path_next;          /* A502 */

    PlayMode mode;
    int      sel;                /* A506: squad under the cursor */
    int      free_slot;          /* A508: unused squad slot, or -1 */
    bool     over_squad;         /* AAE5 */
    int      cx, cy;             /* 9E4A, 9E4C: cursor */
    int      view_top;           /* 886A: line pair at the top of the view */
    bool     trace_toggle;       /* A8C1: the box moves every other iteration */

    int      time_div;           /* A51C: 64 slow, 16 medium, 4 fast, 1 faster */
    int      tick_count;         /* A51A */
    int      ticks;              /* AAE3 */
    int      alert_where;        /* AAE7: where the last "enemy sighted" was */

    int      text_rows;          /* A5C0: text rows below the map */
    int      msg_rows;           /* A5C2: message rows (0-2) */
    char     text[25][41];
    int      trow, tcol;         /* text position (DS:0560, DS:055E) */
    char     cond_mark, food_mark;   /* Info: '+' or '-' after the bars */
    int      trace_start;        /* path slot the box started from */
    int      fight_delay;        /* A6E3: iterations before an encounter becomes a fight (200/400/800) */
    bool     blink;              /* fighting squads blink white and black */
    int      captured_mine, captured_theirs;   /* A6F9, A6FB: men captured */
    int      zoom_squad;         /* Zoom pressed: the squad to fight with (-1 none) */
    bool     prompt_surrender;

    int      key;                /* the key for this iteration */
    int      held_key;           /* cursor key held down */
    bool     jump;               /* Shift held: -/+ jump to the top / bottom */

    uint16_t rnd, rnd_ptr;       /* WAR.EXE random(): DS:0535, DS:0537 */
    uint8_t  terrain[MAP_PIXEL_H][SCREEN_W];   /* the CGA map, for terrain */
    int16_t  supply[MAP_W * MAP_H];   /* 8F74: strength of each side over each map cell (minus = yours) */
    int16_t  opp[OPP_PARAMS];    /* A4DC: the opponent's values (O2) */
    int      alert_timer;        /* A518 */
    int      flags_taken, flags_captured;   /* A703, A701 */
    int      lost_mine, lost_theirs;        /* A6FD, A6FF: men lost */
    bool     reveal;             /* game over: every squad shown (A70F set to 80) */
    bool     prompt_offer;       /* the opponent offers to surrender */
    bool     ended;              /* game over: waiting for Enter */
    bool     enter_icon;         /* "You Won ... [Enter] to continue." */
    PlayResult outcome;          /* result once Enter is pressed */
    PlayResult result;           /* not PLAY_RUNNING once play is over */
    int        result_code;      /* A6F7: 0 won (flags), 1 they surrendered, 2 won (attrition),
                                    10 lost (flags), 11 you surrendered, 12 lost (attrition) */
    bool       prompt_quit;      /* Esc: "Quit to the title page?" */
    int        partner;          /* A508: the other squad for Detach and Join */
    bool       g_join;           /* A5C4: men go to the selected squad (Join) or from it (Detach) */
    int        g_top;            /* A5C6: first row of the Detach/Join table */

    /* the enemy commander (step 5) */
    int16_t  cell_cost[MAP_W * MAP_H];    /* 8B8C: cost of crossing each cell */
    int16_t  cell_value[MAP_W * MAP_H];   /* 9294: worth of each cell (forts, villages, bridges, hills, your flags) */
    int8_t   route[MAP_W * MAP_H];        /* 9B6A: route memory, cell steps 1-8 ending with 99 */
    int      ai_next;            /* A8CD: the squad looked at last */
    int      ai_sq;              /* A50A: the squad being given orders */
    int      home_flag;          /* A510: the enemy flag it guards */
    int      home_cell;          /* A512: its cell */
    int      target_cell;        /* A514: the cell it is going for (-1 none) */
    int      attack_flag;        /* A516: your flag it is going for (-1 none) */
    bool     ai_hold;            /* AAEF: squads on valuable cells stay put */
    int      ai_dest;            /* A8CF: cell to plan a route to */
    int      ai_dist;            /* A8C7: distance found by the last search */
    int      value_fort, value_village, value_bridge, value_flag;   /* A8D1, A8D3, A8D5, A8D7 */

    /* the demo (attract mode) */
    int      demo;               /* A6D8: 0 no, 1 the demo is running, 2 an encounter: Zoom to it */
    int      demo_key;           /* AAED in the demo: the key the demo presses (-/+ scroll, Z) */
    int      demo_wait;          /* the remake: iterations the end of a demo game stays on screen */
} Play;

void play_start(Play *p, const Campaign *c, const GameAssets *ga, uint32_t seed);

/* The demo (show_title_barbarian ovl1:42CC, game_init ovl1:33C9): the game plays
 * itself, fastest, scrolling the map, and zooms into the first encounter.
 * play.result becomes PLAY_QUIT when it is over or a key is pressed. */
void play_start_demo(Play *p, const Campaign *c, const GameAssets *ga, uint32_t seed);

/* A key was pressed (PK_ code); Shift held selects jump scrolling. */
void play_key(Play *p, int key, bool shift);

/* One main-loop iteration. held is the cursor key held down (PK_ code) or 0. */
void play_frame(Play *p, int held);

void play_draw(const Play *p, Screen *s, GfxMode mode);

/* For the battle screen (battle.c) */
int  play_rnd(Play *p);                          /* WAR.EXE random(), 0-127 */
int  play_edge(const Play *p, int i, int j);     /* battle_edge (rt:08EF) */
int  play_tile_under(const Play *p, int i);      /* map tile under squad i */
void play_clear_path(Play *p, int i);
void play_after_battle(Play *p, int a, int b);   /* back on the map after Zoom */
void play_enemy_beaten(Play *p, int en);
void play_resume(Play *p);                       /* back to play after saving (campaign_play again) */         /* ovl7:6EB8: the enemy may give up its target */

#endif
