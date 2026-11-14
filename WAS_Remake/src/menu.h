/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * menu.h: step 6, the pages of overlay 1: "presented by", the title page,
 * the scrolls (titles, story, rules), the opponents, the quote, the campaign
 * menu, Ye Olde Options Shoppe, the opening, the battle report, saving and
 * loading the game (SS), and the demo.
 */
#ifndef WAS_MENU_H
#define WAS_MENU_H

#include "assets.h"
#include "campaign.h"
#include "gfx.h"
#include "play.h"

typedef enum {
    MENU_NONE,          /* stay in the menus */
    MENU_START_GAME,    /* play menu_game() (a copy of the campaign, with its rules and opponent) */
    MENU_RESUME_GAME,   /* the saved game was read: menu_resume() fills the Play */
    MENU_CONTINUE,      /* back to the game after saving */
    MENU_QUIT,          /* leave the program */
    MENU_DEMO,          /* the demo: menu_game() played by the computer (play_start_demo) */
} MenuAction;

typedef struct Menu Menu;

/* data_dir: the remake's data folder (SCNF, DRIVE and the data disks in Data Disks/) */
Menu *menu_create(GameAssets *ga, Campaign *camps, const char *data_dir);
void  menu_destroy(Menu *m);

void  menu_start(Menu *m);                    /* "presented by", then the title page */
void  menu_title(Menu *m);                    /* the title page */
void  menu_report(Menu *m, const Play *p);    /* the battle report, then the title page */
void  menu_save(Menu *m, const Play *p);      /* Alt+S: save the game */

/* WAS.EXE's keyboard: a scan code (PK_...) pressed, and the key let go */
void  menu_key(Menu *m, int key);
void  menu_key_up(Menu *m);
void  menu_shift(Menu *m, bool down);         /* [081A]: Shift held (the ships editor's jumps) */
/* The editors' typing (the story, a title, a ship's name): while this is
 * true, SDL's text input goes to menu_text */
bool  menu_wants_text(const Menu *m);
void  menu_text(Menu *m, const char *utf8);
/* Run overlay 1 for us microseconds of an IBM PC/XT's time */
void  menu_run(Menu *m, double us);
void  menu_draw(const Menu *m, Screen *s, GfxMode mode);
const JrPal *menu_jr(const Menu *m);          /* on a PCjr: the palette registers (rt_0c8f) */

MenuAction menu_action(Menu *m);              /* and clears it */
bool  menu_on_title(const Menu *m);           /* the title page waits for a key (F12: the viewer) */
const Campaign *menu_game(const Menu *m);     /* the campaign to play */
void  menu_random(const Menu *m, uint16_t *rnd, uint16_t *ptr);   /* random()'s state (DS:0808, 080A) */
int   menu_beeps(const Menu *m);              /* sounds asked for (step 8 plays them) */

/* The saved game read from SS: the Play and its campaign (WAS.EXE's DS:3F39-6804) */
bool  menu_resume(Menu *m, Play *p, Campaign *c);

/* The data disk in the drive (for the tests) */
const char *menu_data_dir(const Menu *m);

#endif
