/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * play.h: campaign play on the sea map (overlay 4 of WAS.EXE, campaign_play
 * ovl4:7650), ported routine by routine. Step 2: the map, the marker,
 * scrolling, the command lines, Info, Fleet, Time, Pause and Surrender.
 * The fields keep WAS.EXE's units: x in bytes of 4 pixels, y in line pairs.
 */
#ifndef WAS_PLAY_H
#define WAS_PLAY_H

#include "assets.h"
#include "campaign.h"
#include "gfx.h"

#define PLAY_HZ        27      /* main-loop iterations a second: the marker moves 27 bytes a second
                                  with a key held, measured in DOSBox at 300 cycles (an IBM PC/XT) */
#define VIEW_TOP_MAX   109     /* DS:3F3B at the bottom of the map, in line pairs */
#define VIEW_MARGIN    86      /* the marker stays between the top and top + 86 line pairs */
#define CX_MAX         78      /* the marker's x limit (77 while tracing) */
#define CY_MAX         195     /* its y limit (193 while tracing) */

/* WAS.EXE's scan codes are used as key codes (as in the WAR remake). */
enum {
    PK_NONE = 0, PK_ESC = 0x01, PK_MINUS = 0x0C, PK_EQUALS = 0x0D, PK_TAB = 0x0F,
    PK_E = 0x12, PK_T = 0x14, PK_Y = 0x15, PK_I = 0x17, PK_P = 0x19, PK_ENTER = 0x1C, PK_CTRL = 0x1D,
    PK_S = 0x1F, PK_D = 0x20, PK_F = 0x21, PK_J = 0x24, PK_Z = 0x2C, PK_C = 0x2E,
    PK_N = 0x31, PK_M = 0x32, PK_SPACE = 0x39,
    PK_HOME = 0x47, PK_UP = 0x48, PK_PGUP = 0x49, PK_KP_MINUS = 0x4A, PK_LEFT = 0x4B, PK_KP_5 = 0x4C,
    PK_RIGHT = 0x4D, PK_KP_PLUS = 0x4E, PK_END = 0x4F, PK_DOWN = 0x50, PK_PGDN = 0x51,
    PK_ALT_S = 0x11F,          /* Alt+S: save the game (step 6) */
};

typedef enum {               /* DS:65AA */
    PM_NORMAL = 0,           /* the marker is a cross; Time Fleet Pause Surrender */
    PM_SQUAD  = 1,           /* the marker is over a squadron: its commands are shown */
    PM_TRACE  = 2,           /* tracing a path (step 3) */
    PM_INFO   = 4,           /* Info: the squadron's ships (the original waits in a loop, ovl4:7D69) */
    PM_FLEET  = 97,          /* Fleet: the ships on each side (remake's own number; ovl4:AD63 waits for a key) */
    PM_PAUSE  = 98,          /* Pause (remake's own number; ovl4:7C34 waits for a key) */
    PM_PROMPT = 99,          /* a question, or the game is over: time stops */
} PlayMode;

typedef enum { PLAY_RUNNING, PLAY_WON, PLAY_LOST, PLAY_QUIT, PLAY_SAVE } PlayResult;

typedef struct {
    bool used;               /* 5D2D != 9999 */
    int  side;               /* 5CDD: -1 the player's, 0 the enemy's */
    int  speed;              /* 5D2D: 0 anchored, 1 half sail, 2 full sail */
    int  x, y;               /* 5D7D, 5DCD: byte column, line pair */
    int  supplies;           /* 5E1D */
    int  mark;               /* 5EBD: 0, 1 a flagship aboard (the pennant), 2 merchant */
    int  ctrl;               /* 5EE5: -2 at the start; 0 not under the player's command; -1 always */
    int  heading;            /* 5FD5: 0-15, N clockwise */
    bool battle;             /* 56FF: in an encounter (the crossed swords; Zoom) */
    bool seen;               /* 574F */
    bool has_path;           /* 583F: on a traced path (Info's S changes the speed) */
    bool has_route;          /* 588F: on a route (the enemy) */
    int  port_speed;         /* 6581: the speed before it put into port */
} PSquad;

typedef struct {
    int  sq;                 /* 5E45: its squadron, -1 none */
    int  sails, hull;        /* 5E6D, 6025: damage 0-5 */
    int  type;               /* 5E95: 1 line, 2 frigate, 3 flag */
} PShip;

typedef struct {
    const Campaign   *c;
    const GameAssets *ga;
    PSquad   sq[MAX_SHIPS];
    PShip    ship[MAX_SHIPS];
    int      list[3];        /* 6456: the ships of the squadron last listed (rt_0630), -1 = none */

    PlayMode mode;
    int      sel;            /* 62FB: the squadron under the marker */
    int      free_slot;      /* 62FD: an empty squadron slot, or -1 */
    bool     over_squad;     /* 688C */
    int      cx, cy;         /* 5CC7, 5CC9: the marker */
    int      view_top;       /* 3F3B: the line pair at the top of the view */

    int      time_div;       /* 6312: 64 slow, 16 medium, 4 fast, 1 faster */
    int      tick_count;     /* 6310 */
    int      ticks;          /* 688A */
    int      wind;           /* 6314: 0-3, the wind's direction */

    int      text_rows;      /* 645B: text rows below the map */
    int      msg_rows;       /* 645D: message rows */
    char     text[25][41];
    uint8_t  panel[25][40];  /* text written over the map (Info's panel), shown until the map is drawn again */
    int      trow, tcol;     /* the text position (goto_xy) */
    int      fleet[6];       /* 64C3, 64BF, 64C1, 64C9, 64C5, 64C7: flags, lines, frigates (yours, theirs) */

    int      key;            /* the key for this iteration (DS:6896) */
    bool     jump;           /* Shift held: -/+ jump to the top / bottom (DS:081A) */

    bool     prompt_surrender, prompt_quit;
    bool     ended;          /* the game is over: waiting for Enter */
    PlayResult outcome;      /* the result once Enter is pressed */
    PlayResult result;       /* not PLAY_RUNNING once play is over */
    int      result_code;    /* 649F: 11 you surrendered */
    int      beeps;          /* sounds asked for (step 8 plays them): counted for the tests */
} Play;

void play_start(Play *p, const Campaign *c, const GameAssets *ga);

/* A key was pressed (PK_ code); Shift held selects jump scrolling. */
void play_key(Play *p, int key, bool shift);

/* One main-loop iteration. held is the cursor key held down (PK_ code) or 0. */
void play_frame(Play *p, int held);

void play_draw(const Play *p, Screen *s, GfxMode mode);

/* The height of the map on screen: 200 - 8 x text rows - 2 (182 with two rows) */
int  play_map_h(const Play *p);

#endif
