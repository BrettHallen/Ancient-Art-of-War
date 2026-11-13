/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * campaign.h: the built-in campaigns (S0-SA) from the player's game files.
 *
 * S file (2180 or 2220 bytes), as load_campaign (ovl1:C388) reads it:
 *   0x000  840  story text -> DS:4E53 (a 0x01 byte follows each capital letter)
 *   0x348  200  map -> DS:5BFF: 10 x 20 tile numbers, row by row from the top
 *               (tile number N = tile N-1 in SM / WAST.DAT records 30-31)
 *   0x410  480  squadrons and ships -> DS:5CDD, field by field (campaign.c):
 *               40 squadrons (side, speed, x, y, supplies) and 40 ships
 *               (squadron, sails damage, type)
 *   0x5F0    2  opponent -> DS:646F
 *   0x5F2   18  rules -> DS:64AD: 9 words
 *   0x604  600  ship names -> DS:609F: 40 x 15 bytes
 *   0x85C   40  hull damage per ship -> DS:6025, only in the 2220-byte files
 *               (all zero; the 2180-byte files leave DS:6025 as it was)
 */
#ifndef WAS_CAMPAIGN_H
#define WAS_CAMPAIGN_H

#include "assets.h"
#include "gfx.h"

#define MAP_W          10      /* tiles across */
#define MAP_H          20      /* tiles down */
#define MAP_VIEW_H     182     /* lines of map on screen (the original, checked in DOSBox); text rows 23-24 below */
#define MAP_PIXEL_H    (MAP_H * TILE_H)
#define NUM_CAMPAIGNS  11
#define S_FILE_MIN     0x85C   /* 2140: everything up to the ship names */
#define MAX_SHIPS      40
#define SHIP_NAME_LEN  15
#define NUM_RULES      9
#define NUM_OPPONENTS  6

/* A squadron: 1 to 3 ships that sail together, drawn as one figure on the map */
typedef struct {
    bool    present;               /* word +80 != 9999 */
    bool    friendly;              /* word +0: -1 the player's (white), 0 the enemy's */
    int     x, y;                  /* words +160, +240: byte column, line pair */
    int     supplies;              /* byte +320 (DS:5E1D): 20 when full */
    bool    flagship;              /* one of its ships is a flagship: the pennant (DS:5EBD = 1) */
    int     ships;                 /* how many ships are in it */
} Squadron;

typedef enum { SHIP_LINE = 1, SHIP_FRIGATE = 2, SHIP_FLAG = 3 } ShipType;

typedef struct {
    bool    present;               /* in a squadron (byte +360 != 255) */
    int     squadron;              /* byte +360 (DS:5E45) */
    int     sails_damage;          /* byte +400 (DS:5E6D): 0-5, the Info bar is 5 - damage */
    int     hull_damage;           /* DS:6025: 0-5 */
    int     type;                  /* byte +440 (DS:5E95): 1 line, 2 frigate, otherwise shown as flag (3) */
    char    name[SHIP_NAME_LEN + 1];
} Ship;

typedef struct {
    bool     loaded;
    char     file[4];              /* "S0" ... "SA" */
    char     title[48];
    char     story[841];           /* with the 0x01 markers removed */
    uint8_t  story_raw[840];
    uint8_t  map[MAP_H][MAP_W];
    uint8_t  ships_raw[480];
    uint8_t  hull_raw[MAX_SHIPS];
    Squadron squadrons[MAX_SHIPS];
    Ship     ships[MAX_SHIPS];
    int      num_friendly, num_enemy;            /* squadrons */
    int      ships_friendly, ships_enemy;        /* ships in them */
    int      opponent;
    uint16_t rules[NUM_RULES];
} Campaign;

extern const char *const OPPONENT_NAMES[NUM_OPPONENTS];

/* Load S0-SA and their titles (SITLES). Returns the number of campaigns loaded. */
int  campaigns_load(Campaign out[NUM_CAMPAIGNS], const char *dir);
bool campaign_load_file(Campaign *c, const char *dir, const char *name);
/* The same from an S file's bytes (2180 or 2220; the saved game builds one from SS) */
bool campaign_parse(Campaign *c, const char *name, const uint8_t *d, size_t size);

/* Draw the map tiles: view_h lines from scroll_px down the map. */
void campaign_draw_tiles(Screen *s, const GameAssets *ga, GfxMode mode, const Campaign *c, int scroll_px, int view_h);

/* Draw the map with every squadron as draw_squadrons (rt:10D2) draws one
 * that is not moving: the anchor, white for the player and black for the
 * enemy, with the pennant over a squadron that has a flagship. Every
 * squadron is shown, as when the rules make the enemy always seen. */
void campaign_draw_map(Screen *s, const GameAssets *ga, GfxMode mode, const Campaign *c, int scroll_px);

#endif
