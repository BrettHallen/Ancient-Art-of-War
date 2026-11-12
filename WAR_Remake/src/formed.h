/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * formed.h: step 7, "Teach squads new formations" (WAR.EXE overlay 5 with
 * O8 and the picture PL). Formations 1-5 can be looked at, 6-9 changed.
 */
#ifndef WAR_FORMED_H
#define WAR_FORMED_H

#include "assets.h"
#include "gfx.h"

#define FORMED_MEN 14

typedef struct {
    const GameAssets *ga;
    uint8_t  o8[0x620];       /* O8: the soldiers, their walking scripts and the texts */
    bool     have_o8;
    uint8_t  f[420];          /* DS:A717: the formations being taught (F) */
    int      form;            /* [A8D1]: 1-9 */
    int      mode;            /* [A8C1]: 1 look, 2 the pointer, 3 move a man */
    bool     changed;         /* [A8D3] */
    int      type[FORMED_MEN], x[FORMED_MEN], y[FORMED_MEN];   /* DS:9C32, 9CDA (bytes), 9D12 (line pairs) */
    int      slot[FORMED_MEN];    /* DS:A8D9: the man's place in F */
    int      script[FORMED_MEN];  /* DS:A696: the next step of his script (offset in O8) */
    int      sprite[FORMED_MEN];  /* the sprite drawn (1-6) */
    int      sel;             /* [A506]: the man being moved */
    int      over;            /* [A8C3]: 1 + the man under the pointer, 0 none */
    int      over_was;        /* [A8C5] */
    int      cx, cy;          /* the pointer: [9E4A] bytes, [9E4C] line pairs (+100) */
    int      walk;            /* the direction key for the next step */
    int      tick;
    int      ask;             /* 0, 1 "Save the last changes?", 2 the answer shown */
    bool     answer_yes;
    int      wait;
    int      saving;          /* frames of "Saving Formations" */
    bool     done;
    bool     save;            /* write F (the caller writes the file) */
} FormEdit;

void formed_start(FormEdit *e, const GameAssets *ga, const uint8_t formations[420]);
void formed_key(FormEdit *e, int key);          /* WAR.EXE scan codes (PK_...) */
void formed_frame(FormEdit *e, int held);       /* 20 times a second; held: a cursor key held down */
void formed_draw(const FormEdit *e, Screen *s, GfxMode mode);
bool formed_wants_held(const FormEdit *e);
/* ovl5_45B1: every formation's men in order of x, as F is written */
void formed_sort(uint8_t f[420]);

#endif
