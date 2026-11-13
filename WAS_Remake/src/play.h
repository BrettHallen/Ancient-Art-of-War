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
 * Step 3: the rules: tracing paths, sailing, the wind, supplies, repairs,
 * ports and crowns, rough and shallow water, sighting, encounters and the
 * fights on the map, Detach and Join, and the end of the game.
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
#define NUM_CELLS      200     /* the map: 10 x 20 tiles */
#define PATH_SIZE      1500    /* DS:4E53: the traced paths, a ring of address steps */
#define ROUTE_SIZE     600     /* DS:542F: the routes (the enemy and the merchants, step 5) */
#define PATH_END       99      /* the step that ends a path */
#define NO_DEST        0x270F  /* 9999 */
#define AREA_WORDS     1300    /* DS:3F3D-4964 */
#define MAX_PORTS      75      /* DS:689C: ports a merchant can sail to */
#define DELAY_UNIT_US  50.0    /* one unit of WAS.EXE's delay() on an IBM PC/XT (as WAR's) */

/* WAS.EXE's scan codes are used as key codes (as in the WAR remake). */
enum {
    PK_NONE = 0, PK_ESC = 0x01, PK_MINUS = 0x0C, PK_EQUALS = 0x0D, PK_BACKSPACE = 0x0E, PK_TAB = 0x0F, PK_DELETE = 0x53,
    PK_E = 0x12, PK_T = 0x14, PK_Y = 0x15, PK_I = 0x17, PK_P = 0x19, PK_ENTER = 0x1C, PK_CTRL = 0x1D,
    PK_S = 0x1F, PK_D = 0x20, PK_F = 0x21, PK_J = 0x24, PK_Z = 0x2C, PK_C = 0x2E,
    PK_N = 0x31, PK_M = 0x32, PK_SPACE = 0x39,
    PK_HOME = 0x47, PK_UP = 0x48, PK_PGUP = 0x49, PK_KP_MINUS = 0x4A, PK_LEFT = 0x4B, PK_KP_5 = 0x4C,
    PK_RIGHT = 0x4D, PK_KP_PLUS = 0x4E, PK_END = 0x4F, PK_DOWN = 0x50, PK_PGDN = 0x51,
    PK_ALT_S = 0x11F,          /* Alt+S: save the game (step 6) */
};

typedef enum {               /* DS:65AA, and the remake's numbers for WAS.EXE's waiting loops */
    PM_NORMAL = 0,           /* the marker is a cross; Time Fleet Pause Surrender */
    PM_SQUAD  = 1,           /* the marker is over a squadron: its commands are shown */
    PM_TRACE  = 2,           /* tracing a path: the marker is a box, time stands still */
    PM_INFO   = 4,           /* Info: the squadron's ships (the original waits in a loop, ovl4:7D69) */
    PM_END    = 93,          /* "You Won" / "You Lost" blinking until a key (ovl4:A129) */
    PM_OFFER  = 94,          /* the opponent offers to surrender: Y or N (ovl4:A0E7) */
    PM_DETACH = 95,          /* Detach or Join: moving ships between two squadrons (ovl4:A347) */
    PM_FLEET  = 97,          /* Fleet: the ships on each side (ovl4:AD63 waits for a key) */
    PM_PAUSE  = 98,          /* Pause (ovl4:7C34 waits for a key) */
    PM_PROMPT = 99,          /* a question, or the game is over: time stops */
} PlayMode;

typedef enum { PLAY_RUNNING, PLAY_WON, PLAY_LOST, PLAY_QUIT, PLAY_SAVE } PlayResult;

typedef struct {
    bool used;               /* 5D2D != 9999 */
    int  side;               /* 5CDD: -1 the player's, 0 the enemy's (it flips while fighting) */
    int  speed;              /* 5D2D: 0 anchored, 1 half sail, 2 full sail */
    int  x, y;               /* 5D7D, 5DCD: byte column, line pair; the map address is (y + 1) * 80 + x */
    int  supplies;           /* 5E1D: 0-20 */
    int  mark;               /* 5EBD: 0, 1 a flagship aboard (the pennant), 2 merchant */
    int  ctrl;               /* 5EE5: -2 free; 1.. an encounter's countdown; then -1 or 0 fighting */
    int  heading;            /* 5FD5: 0-15, N clockwise */
    int  partner;            /* 56FF: the other squadron of an encounter + 1 (the crossed swords; Zoom) */
    bool seen;               /* 574F */
    bool has_path;           /* 583F: on a traced path (Info's S changes the speed) */
    int  has_route;          /* 588F: on a route (1; 2 a route to attack, set by ovl4_cfef) */
    int  port_speed;         /* 6581: the speed before an encounter (Info's S sets it too) */
    int  path_pos;           /* 579F: its next step in the path ring */
    int  route_pos;          /* 57EF */
    int  countdown;          /* 56AF: ticks until its next step */
    int  move_delay;         /* 64E1: ticks a step takes */
    int  dest;               /* 6316: where its traced path ends (the little cross), or NO_DEST */
    int  merchant_dest;      /* 5F35: the port a merchant sails for */
    int  routed;             /* 5687: a route was found (1; 2 to a cell where your squadrons are) */
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
    int      free_slot;      /* 62FD: an empty squadron slot (Detach) or Join's partner, or -1 */
    bool     over_squad;     /* 688C */
    int      cx, cy;         /* 5CC7, 5CC9: the marker */
    int      view_top;       /* 3F3B: the line pair at the top of the view */

    int      time_div;       /* 6312: 64 slow, 16 medium, 4 fast, 1 faster */
    int      tick_count;     /* 6310 */
    int      ticks;          /* 688A: 0-30, what happens on ticks 10, 20, 25 and 31 */
    int      sq_turn;        /* campaign_play's l2: the next squadron to eat (ovl4_9345) */
    int      wind;           /* 6314: 0-3, the wind blows from N, E, S, W */
    bool     wind_on;        /* 6865: the Options Shoppe's wind */
    int      fight_delay;    /* 6851: ticks before an encounter is fought out on the map (250, 500, 1000) */

    /* the map */
    uint8_t  map[NUM_CELLS];        /* DS:5BFF */
    /* DS:3F3D-4964, kept in WAS.EXE's order: the route searches' work arrays
       run on into each other when they overflow, as in WAS.EXE */
    union {
        int16_t area[AREA_WORDS];
        struct {
            int16_t  came[NUM_CELLS];      /* 3F3D: the step a search came into each cell by (9 the start) */
            int16_t  cost[NUM_CELLS];      /* 40CD: its cost so far */
            uint16_t coast[NUM_CELLS];     /* 425D: the shape of each cell's shore (rt_2523) */
            int16_t  danger[NUM_CELLS];    /* 43ED: the extra cost of a cell near your strength */
            int16_t  queue[100];           /* 457D: the search's cells */
            int16_t  strength[NUM_CELLS];  /* 4645: the strength map (ovl4_e805): theirs +, yours -,
                                              -7 your flagship */
            int16_t  pvalue[NUM_CELLS];    /* 47D5: what the cells beside ports and crowns are worth
                                              to the enemy (ovl4_ea51) */
        };
    };
    int      port_sup[NUM_CELLS];   /* 4965: what a port or crown has left */
    bool     inverted[NUM_CELLS];   /* the tile is drawn inverted (rt_2e61): a port with nothing left */
    int      port_cell[MAX_PORTS];  /* 689C: ports merchants can reach, -1 after the last */
    int      port_source[MAX_PORTS];/* 6932: where on the edge of the map their merchants come from */
    int      ports_supply, ports_repair, crowns;   /* 6499, 649B, 649D */

    /* the paths */
    int8_t   path[PATH_SIZE + ROUTE_SIZE];
    int      path_w;         /* 62F7: where tracing writes */
    int      trace_skip;     /* 64D1: the marker moves every other iteration while tracing */

    int      rules[NUM_RULES];  /* 64AD-64BD: repairs, port supplies, ?, rough water, eating,
                                   visibility, shallow water, wind changes, ? */
    int16_t  opp[OPP_WORDS];    /* 604D-607D: the opponent's values (SO2) */

    int      text_rows;      /* 645B: text rows below the map */
    int      msg_rows;       /* 645D: message rows */
    int      msg_last;       /* 6890: the last message, not repeated */
    int      msg_timer;      /* 630D */
    char     text[25][41];
    uint8_t  panel[25][40];  /* text written over the map (Info, Detach, the offer), shown in those modes */
    struct { uint16_t off; int8_t x; } icon[25][2];   /* icons on the text rows (rt_2cc4) */
    int      trow, tcol;     /* the text position (goto_xy) */
    int      fleet[6];       /* 64C3, 64BF, 64C1, 64C9, 64C5, 64C7: flags, lines, frigates (yours, theirs) */

    int      dist;           /* 689A: the distance between the last two squadrons compared */
    int      losses[4];      /* 64A5, 64A7, 64A9, 64AB: your ships, theirs, your flagships, theirs */
    int      captured[2];    /* 64A1, 64A3: your ships taken by the enemy, theirs taken by you (step 4) */
    int      battle_req;     /* Zoom (1) or Practice (2) asked for: main.c starts the sea battle */
    int      battle_shore;   /* 686B: the shore in the battle, from the map (ovl4:7EEA) */
    int      battle_a, battle_b;   /* 6301, 6303: the last Zoom's squadrons (step 4) */
    int      crown_threat;   /* 6309 */
    int      merchant_lost;  /* 6898 */
    /* the enemy (step 5) */
    int      ai_done;        /* 64DD: the last squadron the enemy gave orders to (move_squadrons sets it
                                back to look again at one that finished its path) */
    int      ai_sq;          /* 62FF: the squadron being given orders */
    int      ai_range;       /* 64D7: the distance to the squadron chosen to attack, the path's limit */
    int      ai_target;      /* 64DF: the cell a route is looked for to */
    int      qn;             /* 6495: cells in the search queue */
    int      ai_reverse;     /* 69CB: the routes to the target set out the other way round */
    int      ai_dropped;     /* 69CD: the target given up, not to be taken again at once */
    int      water_value[2]; /* 65B2, 65B4 (rt_2523, for step 5) */
    bool     in_encounters;  /* inside ovl4_8BCD: an accepted surrender does not end the game */
    bool     no_flags[2];    /* 65BA, 65BB: you, they began with no flagship but have a crown:
                                losing every flagship does not end the game */
    bool     zoomed;         /* 630F: set by a sea battle (step 4); the end codes become 14 and 4 */

    /* Detach and Join (ovl4:A347) */
    int      dj_other[3];    /* 69C8: the ships of the second squadron */
    int      dj_pos;         /* the ship marked: 0-2 the first squadron, 3-5 the second */
    bool     dj_new;         /* the second squadron was empty (Detach) */
    PlayMode back_mode;      /* the mode Detach, Join or the offer goes back to */

    /* the end */
    bool     offer_ends;     /* Y to the offer ends the game */
    bool     offer_at_start; /* the offer came as the game began (over the menus in WAS.EXE) */
    int      end_won;        /* ovl4:A129's argument: 1 lost, 0 or 2 won */
    int      end_frame;
    int      end_key;        /* a key pressed during the blinking, read at the end of a blink */
    bool     end_quit;       /* the game ends straight after the key (ovl4:C51A) */

    uint16_t rnd, rnd_ptr;   /* random() (rt:2F0D): DS:0808 and 080A */
    int      stall;          /* iterations WAS.EXE spends in delay() or working (play_frame) */
    double   work_us;        /* the work's XT time not yet turned into stalled iterations */
    int      tick_stage;     /* a tick under way (play_frame): its next part, 0 none */
    double   idle_left;      /* the idle iteration's work not yet set against this frame's */

    int      key;            /* the key for this iteration (DS:6896) */
    bool     jump;           /* Shift held: -/+ jump to the top / bottom (DS:081A) */
    int      keybuf[15];     /* keys pressed while WAS.EXE was busy, as in the BIOS's buffer */
    bool     keybuf_shift[15];
    int      keybuf_n;

    bool     prompt_surrender, prompt_quit;
    bool     ended;          /* the game is over: waiting for Enter */
    PlayResult outcome;      /* the result once Enter is pressed */
    PlayResult result;       /* not PLAY_RUNNING once play is over */
    int      result_code;    /* 649F: 0, 2, 3, 4 won; 10, 12, 13, 14 lost; 1 their surrender accepted; 11 yours */
    int      beeps;          /* sounds asked for (step 8 plays them): counted for the tests */

    /* the demo (step 6): [6493] 1, 2 once an encounter is announced (Zoom next);
       the key it plays ([6896], keypad - or +: the map scrolls) */
    int      demo;
    int      demo_key;
    int      demo_end;       /* iterations the end of a demo game stays */
    bool     hidden;         /* campaign_play is still loading: the last screen stays (main.c) */
} Play;

/* Start a new game of campaign c. seed: the random number state (WAS.EXE
 * takes it from the clock). */
void play_start(Play *p, const Campaign *c, const GameAssets *ga, unsigned seed);
/* The same with random()'s state carried over from the menus (DS:0808, 080A) */
void play_start_rnd(Play *p, const Campaign *c, const GameAssets *ga, uint16_t rnd, uint16_t ptr);
/* The demo (ovl1:87FF): opponent 2's values with 604F 3 and 6057 128, the
 * fastest time, every squadron seen; the computer scrolls and zooms */
void play_start_demo(Play *p, const Campaign *c, const GameAssets *ga, uint16_t rnd, uint16_t ptr);
/* campaign_play starts again after the game was saved or loaded (ovl4:7650
 * with [6465] = 0) */
void play_resume(Play *p);

/* A key was pressed (PK_ code); Shift held selects jump scrolling. */
void play_key(Play *p, int key, bool shift);

/* One main-loop iteration. held is the cursor key held down (PK_ code) or 0. */
void play_frame(Play *p, int held);
bool play_busy(const Play *p);      /* WAS.EXE working: a stall, or a tick under way */

void play_draw(const Play *p, Screen *s, GfxMode mode);
void play_draw_offer(const Play *p, Screen *s, GfxMode mode);   /* over the menus' last page */

/* The height of the map on screen: 200 - 8 x text rows - 2 (182 with two rows) */
int  play_map_h(const Play *p);

/* The squadron's map cell (squadron_cell rt:1985) */
int  play_cell(const Play *p, int sq);

/* For the sea battle (battle.c) */
int  play_random(Play *p);                       /* random() rt:2F0D */
void play_list_ships(Play *p, int sq);           /* rt_0630 into p->list */
void play_clear_paths(Play *p, int sq);          /* rt_0f55 */
void play_place_detached(Play *p, int from, int to, int steps, int at);   /* rt_1f2d */
/* campaign_play starts again after a battle (ovl4:7650 with [6465] = 0) */
void play_after_battle(Play *p, bool practice, bool left);

/* The map maker's check (ovl3_8146): 1 the map will do, 0 a port or crown
 * has no way by sea to the map's edge ("ALL PORTS MUST HAVE A SUPPLY PATH."),
 * -1 one has no water beside it ("ALL PORTS MUST BE NEXT TO WATER.").
 * *steps: the cells and directions looked at (for the time it takes). */
int  play_map_check(const GameAssets *ga, const uint8_t map[NUM_CELLS], long *steps);

#endif
