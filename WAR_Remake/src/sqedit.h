/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * sqedit.h: step 7, putting squads and flags on the map (WAR.EXE overlay 3
 * with O1), the second step of making a campaign and the ARMY of changing one.
 */
#ifndef WAR_SQEDIT_H
#define WAR_SQEDIT_H

#include "assets.h"
#include "campaign.h"
#include "gfx.h"
#include "play.h"

#define EDIT_HZ 20            /* sqedit_frame calls a second (the menus' rate) */

typedef enum {
    SQEDIT_RUNNING = -1,
    SQEDIT_NEXT    = 0,       /* a new campaign: on to the next step */
    SQEDIT_SAVE    = 100,     /* changing a campaign: save the changes */
    SQEDIT_DISCARD = 101,     /* changing a campaign: forget them */
} SqEditResult;

typedef struct {
    const GameAssets *ga;
    uint8_t  o1[0x260];       /* O1: the editor's texts */
    bool     have_o1;
    bool     new_campaign;    /* [A692] = 9 */
    Campaign c;               /* the campaign being edited (every squad seen) */
    Play     p;               /* the map, squads, flags, cursor and text rows, drawn as in the game */
    int      state;
    int      flag_sel;        /* [A504]: the flag under the cursor */
    bool     over_squad;      /* [A8CB] */
    bool     over_flag;       /* [A8C9] */
    bool     increase;        /* Size and Info: the keys add ([bp-5], [A8C7]) */
    int      wait;            /* frames left of a pause */
    bool     answer_yes;
    SqEditResult result;
} SquadEdit;

/* c: the campaign to change; for a new one, its map and the blank armies */
void sqedit_start(SquadEdit *e, const GameAssets *ga, const Campaign *c, bool new_campaign);
void sqedit_key(SquadEdit *e, int key);        /* WAR.EXE scan codes (PK_...) */
void sqedit_frame(SquadEdit *e, int held);     /* held: the cursor key held down */
void sqedit_draw(const SquadEdit *e, Screen *s, GfxMode mode);
/* The armies as edited, back into the campaign (armies[] and squads[]) */
void sqedit_store(const SquadEdit *e, Campaign *c);

#endif
