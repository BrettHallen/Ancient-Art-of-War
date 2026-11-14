/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * battle.h: step 4, the sea battle (Zoom and Practice: WAS.EXE overlays 5
 * and 6 and the resident battle routines rt:3580-6765) and boarding
 * (overlay 7).
 *
 * WAS.EXE's battle code draws straight onto the CGA screen and waits with
 * delay() in the middle of what it is doing (a broadside, a ship blinking,
 * the explosion). The remake runs the ported code as it is written, on a
 * fiber (fiber.h): delay() and the work of each main-loop iteration cost
 * time on a clock of its own, and the battle hands control back to the
 * main loop whenever it is ahead of real time. The screen is kept as the
 * 16 KB of CGA memory WAS.EXE draws into (CGA rows and bytes), and the
 * battle's data as a copy of WAS.EXE's data segment (with SO6, SO7, SO8
 * and the shore loaded where WAS.EXE puts them), so the pointers in its
 * tables are kept as they are.
 *
 * Units, as WAS.EXE's: a ship's x and y are "battle units"; the overview
 * (View) draws them at x/6 bytes and y/6 line pairs, the close view at
 * x - scroll bytes and y - scroll line pairs.
 */
#ifndef WAS_BATTLE_H
#define WAS_BATTLE_H

#include "fiber.h"
#include "gfx.h"
#include "play.h"

#define BS          6        /* ships in a battle: 0-2 yours (squadron 6303), 3-5 theirs (6301) */
#define BPART       18       /* DS:6B19: smoke and splashes, three sets of six */
#define BSHOTS      13       /* DS:6C19 / 6C33 */
#define NOSHIP      (-999)   /* 0FC19h: no ship in this place */
#define BMEM_SO8    0x8000   /* where the remake puts SO8 in its copy of the data segment ([6489]) */
#define BMEM_SHORE  0x7000   /* the shore, run-length coded ([6CD5], 32Fh bytes) */
#define BATTLE_ITER_US 0     /* the rest of an iteration's work, beyond the drawing (see battle.c) */

typedef enum { BATTLE_RUNNING, BATTLE_OVER, BATTLE_QUIT } BattleState;

typedef struct Battle {
    Play    *p;
    const GameAssets *ga;
    uint8_t  m[0x10000];      /* WAS.EXE's data segment: tables, SO6 (4AF5), SO7 (4257), SO8, the shore,
                                 the gun crew's panel buffer (3F3D, 44 bytes x 18 lines) */
    uint8_t  vram[0x4000];    /* the CGA screen (B800:0000): even lines, then odd lines at 2000h */
    int      palette;         /* CGA palette 1 (normal) or 0 (the explosion's flashes) */
    JrPal    jr;              /* on a PCjr: the palette registers (rt_0c8f, then the BIOS's flashes) */

    /* The Tandy version (WAST.EXE, the remake's 16-colour mode). Its battle is
       WAS.EXE's in the CGA 4-colour mode, so every drawing routine also draws
       into vram_t, with WAST.EXE's fills (the sea 55h, the shore lines 55h,
       AAh, 5Ah, A5h) and its own SO6 and SO7; shown with the palette registers
       tpal: the registers treg (sea_battle ovl4:0037: 0, 9 light blue, 6 brown,
       15), or during the explosion's flashes with the BIOS's palette 0 0, 10,
       12, 14 (seen in DOSBox-X: palette 1 shows treg again). Boarding is
       in the 16-colour mode 9 (boarding_load_b ovl4:72A2): the deck from
       WAST.DAT, the men turned to 4 bits a pixel (expand_2to4 rt:01DAB). */
    uint8_t  vram_t[0x4000];
    uint8_t  tpal[4], treg[4];
    uint8_t  so6t[0x355], so7t[0x70E], panel_t[0x2C * 18];
    bool     t9;                              /* boarding: the screen is mode 9 */
    uint8_t  deck16[2][100 * 160];            /* WAST.DAT's deck, by field, 160 bytes a line */
    uint8_t  work16[2][100 * 160];            /* the deck with the men (unpack_deck ovl4:9EF6, draw_man A006) */
    uint8_t  scr16[SCREEN_W * SCREEN_H];      /* mode 9's screen (deck_to_screen ovl4:9F90) */
    bool     row16[SCREEN_H];                 /* rows of scr16 shown (the rest: vram_t, the text) */

    /* the fiber and its clock */
    Fiber    fiber;
    double   clock_us;        /* the battle's own time */
    double   target_us;       /* run until the clock passes this */
    int      keys[64];        /* the keyboard buffer, as read by rt_2e8e (DOS 08h) */
    int      key_head, key_tail;
    BattleState state;
    int      result;          /* ovl5_7650's return: 0, or 9999 */
    bool     practice;        /* [65A9] */
    int      beeps;           /* speaker sounds made (the tests count them) */
    bool     spk;             /* the speaker is on (speaker_on, till rt_2f67) */
    int      saved_6867;      /* rt_0000: the guns' option, 10 during Practice */

    /* ships (word arrays [i], byte arrays {i} in WAS.EXE) */
    int type[BS];            /* 69E1{}: 1 line, 2 frigate, 3 flag */
    int x[BS], y[BS];        /* 69E7[], 69F3[]: NOSHIP when gone */
    int owner[BS];           /* 69FF[]: -1 yours, 0 theirs (boarding changes it) */
    int sails[BS], hull[BS]; /* 6A0B{}, 6A11{}: damage; hull 6-7 struck, 8 and up sinking */
    int order[BS];           /* 6A17[]: 1 course, 2 mimic, 3 follow, 4 general chase, 5 avoid battle, 6 board; 888 gone */
    int sail_set[BS];        /* 6A23[]: the sail ordered, 0-6 */
    int xcount[BS], ycount[BS];   /* 6A2F[], 6A3B[]: iterations to the next step east/west, north/south */
    int speed[BS];           /* 6A47[]: the speed, 0-9 */
    int speed_wait[BS];      /* 6A53[] */
    int turn_wait[BS];       /* 6A5F[] */
    int moved[BS];           /* 6A6B[]: the last step (to erase the old picture), 999 none */
    int aim_r[BS], aim_r_set[BS];   /* 6A77[], 6A83[]: starboard guns' elevation fired / set (0-3) */
    int aim_l[BS], aim_l_set[BS];   /* 6A8F[], 6A9B[]: port guns' */
    int crew_side[BS];       /* 6AA7[]: the gun crew at the port (0) or starboard (1) guns */
    int gun_l[BS], gun_r[BS];/* 6AB3[], 6ABF[]: loading, 13 = ready */
    int heading[BS];         /* 6ACB[]: 0-15, N clockwise */
    int turn[BS];            /* 6AD7[]: steps of turn still to make (+ right, - left) */
    int turning[BS];         /* 6AE3[]: half way through a turn */
    int half[BS];            /* 6AEF{}: drawn half a byte (2 pixels) to the right */
    int sub[BS];             /* 6AF5{} */
    int target[BS];          /* 6AFB{}: the ship it follows or chases, -1 */
    int anim_from[BS];       /* 6B01[] */
    int draw_order[BS];      /* 6B0D{}: by y */
    int grappled[BS];        /* 6B13{}: the ship it is fast to, -1 */
    int prev_heading[BS];    /* 6C4D{}: the heading being turned from */
    int name[BS];            /* 6C53[]: DS offset of the name (the campaign's ship names) */
    int dd[BS];              /* 6CDD[] */
    int ship_no[BS];         /* the campaign's ship in this place (6456 lists) */

    /* smoke and splashes */
    int pstate[BPART], pnext[BPART], powner[BPART], pvar[BPART], px[BPART], py[BPART];   /* 6B19 6B2B 6B3D 6B4F 6B61 6B85 */
    int shot_y[BSHOTS], shot_x[BSHOTS];    /* 6C19[], 6C33[] */

    /* sprite pointers (DS offsets) */
    int heading_spr[16];     /* DS:607F: the map's ship sprites, or SO8's in the close view */
    int spr_frigate_turn[16], spr_ship[16], spr_ship_turn[16];   /* 6BA9, 6BC9, 6BE9 */
    int crew_spr[32];        /* 6C97-6CD3: the gun crew's pictures, as the scripts number them */
    int gun_l_spr[4], gun_r_spr[4], aim_l_spr[4], aim_r_spr[4];  /* 6C11, 6C09, 6C5F, 6C67 */
    int tbl_turn, tbl_turn2, tbl_skill, tbl_pos_l, tbl_pos_r;    /* 6C7D, 6C7F, 6C81, 6C83, 6C85 */
    int boom_spr[4];         /* 6C6F-6C75 */

    /* the battle */
    int sel;                 /* 69D1: your ship shown in the panel */
    int menu;                /* 69D3: 'A' aim, 'C' course, 'S' speed, 'O' orders, 'P' pause, 'B' grappled */
    int time;                /* 69CF: 0 faster ... 4 slower */
    int view;                /* 69DD: 2 the overview, 3 close */
    int vx, vy;              /* 69D5, 69D7: the close view's top left */
    int bx, by;              /* 69D9, 69DB: the box in the overview */
    int shore;               /* 686B: 0 open sea, 1-8 a shore, from the map cell */
    int crew_x;              /* 6C77: where the gun crew's man stands in the panel buffer */
    int crew_scroll;         /* 6C7B: 0 the port guns shown, 20 the starboard */
    int script;              /* 6C79: the gun crew's animation script (DS offset) */
    int ending;              /* 6CD9: iterations until the battle ends */
    int msg_timer;           /* 6C95 */
    bool no_redraw;          /* 6C94 */
    int ai_ship;             /* 686E */
    int fired_sunk, fired_struck;   /* 6CDC, 6CDB */
    int sunk_ship, sunk_how; /* 6C8F, 6C91: 1 heavy fire, 2 collision, 3 aground, 10 */
    int new_sq[2], new_n[2]; /* 6CED / 6CEF, 6CE9 / 6CEB: squadrons for captured ships */
    bool view_moved;         /* 6C8B */
    bool follow;             /* 6C89 */
    bool enemy_fired;        /* 6C8D */
    bool flag_tune;          /* 6C93 */
    bool left;               /* 65B8: Leave */
    int  nibble;             /* 6CD7 */
    int  trow, tcol;         /* the text position (DS:833, 831) */
    int  sq_a, sq_b;         /* 6301 (theirs), 6303 (yours) */
    bool demo;               /* [6493] (step 6) */
    int  joy;                /* [648D]: the joystick's command bar (the remake has no joystick) */
    int  stale;              /* ovl5_bc12's local at bp-24h, read by rt_5199(17) before it is set */
} Battle;

/* Start a battle for the squadron under the marker (Zoom, or Practice). The
 * battle runs on its own fiber. False if SO6-SO8 are missing. */
bool battle_start(Battle *b, Play *p, bool practice);
/* A key: the character DOS function 08h returns (letters, digits, Enter 13,
 * Esc 27, Space 32), or 0 then the scan code for the keypad and cursor keys
 * (battle_key(b, 0, scan) queues both). */
void battle_key(Battle *b, int ch, int scan);
/* Run the battle for us microseconds of real time. */
void battle_run(Battle *b, double us);
/* Draw the CGA screen into s. */
void battle_draw(const Battle *b, Screen *s, GfxMode mode);
/* After the battle (state != BATTLE_RUNNING): free the fiber and go back to
 * the map (campaign_play starts again: Play's after-battle checks). */
void battle_finish(Battle *b);

/* For the tests only: when >= 0, every broadside blows up this ship as though a
 * shot had (as WAS_Analysis/tools/dosbox's patched WAST.EXE did to see the
 * Tandy explosion's colours). -1 in the game. */
extern int battle_test_blow_up;

#endif
