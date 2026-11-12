/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * menu.h: step 6, the title pages, scrolls, menus, Options Shoppe, saved
 * games and the battle report (WAR.EXE overlay 1).
 */
#ifndef WAR_MENU_H
#define WAR_MENU_H

#include "assets.h"
#include "campaign.h"
#include "gfx.h"
#include "play.h"

#define MENU_HZ 20

typedef enum {
    MENU_NONE,          /* stay in the menus */
    MENU_START_GAME,    /* play menu.game (a copy of the campaign, with its rules and opponent) */
    MENU_RESUME_GAME,   /* play the saved game loaded into the Play given to menu_resume_load */
    MENU_CONTINUE,      /* back to the game after saving */
    MENU_QUIT,          /* leave the program */
    MENU_VIEWER,        /* the remake's asset viewer */
    MENU_DEMO,          /* the demo: menu.game played by the computer (play_start_demo) */
} MenuAction;

typedef struct {        /* the options in CNF (25 bytes from DS:A6DE) */
    uint8_t raw[25];
    int display;        /* A6DF: 0 black/white, 1 tv/composite, 2 rgb */
    int sounds;         /* A6E1: 0 off, 1 no music, 2 on, 3 external speaker */
    int fight_delay;    /* A6E3: 200, 400, 800 */
    int joystick;       /* A6E7 */
    int data_drive;     /* A6F5: 0 A:, 1 B: */
} MenuOptions;

typedef struct Menu Menu;

/* data_dir: the data folder (CNF, F, DRIVE, and the data disks in Data Disks/) */
Menu *menu_create(GameAssets *ga, Campaign *camps, const char *data_dir);
void  menu_destroy(Menu *m);

void  menu_start(Menu *m);                    /* "presented by", then the title page */
void  menu_title(Menu *m);                    /* the title page */
void  menu_report(Menu *m, const Play *p);    /* the battle report, then the title page */
void  menu_save(Menu *m, const Play *p);      /* Alt+S: save the game */

void  menu_key(Menu *m, int key);             /* WAR.EXE scan codes (PK_...) */
void  menu_frame(Menu *m);                    /* MENU_HZ times a second */
void  menu_draw(Menu *m, Screen *s, GfxMode mode);

/* step 7, the editors: the squads editor follows a held cursor key, the
 * scroll editor takes typed text */
void  menu_held(Menu *m, int held);           /* each frame, before menu_frame */
bool  menu_wants_held(const Menu *m);
bool  menu_wants_text(const Menu *m);
void  menu_text(Menu *m, const char *utf8);   /* SDL text input */

MenuAction menu_action(Menu *m);              /* and clears it */
const Campaign *menu_game(const Menu *m);     /* the campaign to play */
const MenuOptions *menu_options(const Menu *m);
uint32_t menu_seed(const Menu *m);

/* Saved games (the remake's own format, in the data folder) */
bool  menu_has_saved_game(const Menu *m);
bool  menu_load_saved_game(Menu *m, Play *p, Campaign *c);
void  menu_saved_game_failed(Menu *m);        /* why S can't be continued, then the title page */

#endif
