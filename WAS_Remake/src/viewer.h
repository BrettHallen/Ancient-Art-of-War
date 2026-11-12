/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * viewer.h: the asset viewer (step 1): every picture, the map tiles, the
 * font, the map sprites and the eleven campaign maps (with each one's story
 * and ships), in CGA or 16 colours. Enter on a map plays that campaign (main.c).
 */
#ifndef WAS_VIEWER_H
#define WAS_VIEWER_H

#include "assets.h"
#include "campaign.h"
#include "gfx.h"

enum {
    SCREEN_TILES = PIC_COUNT,
    SCREEN_FONT,
    SCREEN_SPRITES,
    SCREEN_MAP_FIRST,
    NUM_SCREENS = SCREEN_MAP_FIRST + NUM_CAMPAIGNS
};

#define SCROLL_BOTTOM (MAP_PIXEL_H - MAP_VIEW_H)

typedef struct {
    const GameAssets *ga;
    const Campaign   *campaigns;      /* NUM_CAMPAIGNS */
    GfxMode           mode;
    int               index;          /* the screen shown */
    int               scroll[NUM_CAMPAIGNS];   /* pixels from the top of each map */
    bool              caption;
    int               page;           /* on a map: 0 the map, 1 the story, 2 the ships */
} Viewer;

/* Viewer keys (SDL keycodes are mapped in main.c) */
typedef enum {
    VK_NONE, VK_NEXT, VK_PREV, VK_FIRST, VK_UP, VK_DOWN, VK_TOP, VK_BOTTOM,
    VK_MODE, VK_CAPTION, VK_STORY
} ViewerKey;

void viewer_init(Viewer *v, const GameAssets *ga, const Campaign *campaigns);
void viewer_key(Viewer *v, ViewerKey k);
void viewer_scroll(Viewer *v, int delta);          /* on a map, in pixels */
bool viewer_is_map(const Viewer *v);
const char *viewer_title(const Viewer *v, int index);
void viewer_draw(const Viewer *v, Screen *s);

#endif
