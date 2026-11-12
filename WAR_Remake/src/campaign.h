/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * campaign.h: the built-in campaigns (T0-TA) from the player's game files.
 *
 * T file (1460 bytes):
 *   0x000  840  story text (a 0x01 byte follows each capital letter)
 *   0x348  200  map: 10 x 20 tile numbers, row by row from the top
 *               (tile number N = tile N-1 in M / WARET.DAT record 30)
 *   0x410  400  armies: 20 squads and 10 flags, stored field by field
 *               (see campaign.c; decoded from the squad editor, overlay 3)
 *   0x5A0    2  opponent (0-7)
 *   0x5A2   18  rules: 9 words
 */
#ifndef WAR_CAMPAIGN_H
#define WAR_CAMPAIGN_H

#include "assets.h"
#include "gfx.h"

#define MAP_W          10      /* tiles across */
#define MAP_H          20      /* tiles down */
#define MAP_VIEW_ROWS  9       /* rows visible at once (180 pixels) */
#define MAP_VIEW_H     (MAP_VIEW_ROWS * TILE_H)
#define MAP_PIXEL_H    (MAP_H * TILE_H)
#define NUM_CAMPAIGNS  11
#define T_FILE_SIZE    1460

typedef enum {
    RULE_VILLAGES_SUPPLY_FOOD, RULE_FORTS_SUPPLY_FOOD, RULE_SUPPLY_LINE_RANGE, RULE_FORTS_TRAIN_MEN,
    RULE_MEN_SHAPE, RULE_ENEMY_SEEN, RULE_WATER, RULE_MOUNTAIN, RULE_FOREST, RULE_COUNT
} RuleId;

#define MAX_SQUADS 20
#define MAX_FLAGS  10

typedef struct {
    bool present;
    bool friendly;     /* the player's (white) squads */
    int  x, y;         /* top-left on the 320x400 map, in pixels */
    int  knights, barbarians, archers, spies;
    int  speed;        /* 1 = march fast, 2 = march, 3 = march slow */
    int  formation;    /* 1-9 */
    int  condition;
    int  food;
} Squad;

typedef struct {
    bool present;
    bool white;        /* the player's flag */
    int  x, y;         /* top-left on the map, in pixels */
} Flag;

typedef struct {
    bool     loaded;
    char     file[4];          /* "T0" ... "TA" */
    char     title[48];
    char     story[841];       /* with the 0x01 markers removed */
    uint8_t  story_raw[840];   /* as in the T file: 26 rows of 32, 0x01 after each capital */
    uint8_t  map[MAP_H][MAP_W];
    uint8_t  armies[400];
    Squad    squads[MAX_SQUADS];
    Flag     flags[MAX_FLAGS];
    int      num_friendly, num_enemy;
    int      opponent;
    uint16_t rules[RULE_COUNT];
} Campaign;

extern const char *const OPPONENT_NAMES[8];

/* Load T0-TA and the titles. Returns the number of campaigns loaded. */
int  campaigns_load(Campaign out[NUM_CAMPAIGNS], const char *dir);
/* One T file (T0-TB) from a folder: the WAR disk, or the data disk's campaigns. */
bool campaign_load_file(Campaign *c, const char *dir, const char *name);

/* Write a T file (1460 bytes) to a folder: what ovl1_7A71 writes. */
bool campaign_save_file(const Campaign *c, const char *dir, const char *name);
/* After an editor has changed armies[] or story_raw[]: the squads, flags and story again. */
void campaign_refresh(Campaign *c);

/* Draw just the map tiles: view_h lines from scroll_px down the map. */
void campaign_draw_tiles(Screen *s, const GameAssets *ga, GfxMode mode, const Campaign *c, int scroll_px, int view_h);

/* Draw the map, squads and flags, with the top of the view scroll_px pixels
 * down the map. Every squad is drawn in its starting pose (standing). */
void campaign_draw_map(Screen *s, const GameAssets *ga, GfxMode mode, const Campaign *c, int scroll_px);

#endif
