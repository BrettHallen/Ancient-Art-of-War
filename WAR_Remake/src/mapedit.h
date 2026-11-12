/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * mapedit.h: step 7, the map maker (WAR.EXE overlay 4 with O0), the first
 * step of making a campaign and the MAP MAKER of changing one.
 */
#ifndef WAR_MAPEDIT_H
#define WAR_MAPEDIT_H

#include "assets.h"
#include "campaign.h"
#include "gfx.h"

typedef enum {
    MAPEDIT_RUNNING = -1,
    MAPEDIT_NEXT    = 0,      /* a new campaign: on to the next step */
    MAPEDIT_ABORT   = 5,      /* a new campaign: Esc at the start */
    MAPEDIT_SAVE    = 100,    /* changing a campaign: save the changes */
    MAPEDIT_DISCARD = 101,    /* changing a campaign: forget them */
} MapEditResult;

typedef struct {
    const GameAssets *ga;
    uint8_t  o0[0x1B0];       /* O0: texts and the A, S and Esc key icons */
    bool     have_o0;
    bool     new_campaign;    /* [A692] = 3: the intro, a fresh map, "Continue to the next step?" */
    uint8_t  map[MAP_H][MAP_W];   /* DS:9D82 */
    int      phase;           /* 0 intro, 1 editing, 2 the question */
    int      row, col;        /* [A8C5], [A8C3]: the cursor's map cell */
    int      top;             /* [A8C1]: first map row in the view */
    int      page;            /* [A8C9]: 0-3, 18 tiles each */
    bool     answer_shown;    /* "Yes" / "No" written: the result is ready */
    int      wait;            /* frames before the result is passed on */
    MapEditResult result;
} MapEdit;

void mapedit_start(MapEdit *e, const GameAssets *ga, const Campaign *c, bool new_campaign);
void mapedit_key(MapEdit *e, int key);         /* WAR.EXE scan codes (PK_...) */
void mapedit_frame(MapEdit *e);
void mapedit_draw(const MapEdit *e, Screen *s, GfxMode mode);

#endif
