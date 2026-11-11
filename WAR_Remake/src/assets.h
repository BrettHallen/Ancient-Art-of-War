/*
 * The Ancient Art of War - remake
 * assets.h: loads graphics from the player's own copy of the original game.
 *
 * CGA set: WAR.EXE (font), M (map tiles), P0-PO (pictures)
 * VGA set: WARET.DAT (16-colour pictures and map tiles)
 */
#ifndef WAR_ASSETS_H
#define WAR_ASSETS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SCREEN_W   320
#define SCREEN_H   200
#define NUM_TILES  72
#define TILE_W     32
#define TILE_H     20
#define FONT_FIRST 0x20   /* first character in the font (space) */
#define FONT_COUNT 95     /* space .. '~' */
#define NUM_BATTLE 14     /* battle backgrounds P0-PD */
#define SPRITE_W   8      /* map sprites (squads, flags) */
#define SPRITE_H   12

typedef struct { uint8_t r, g, b; } RGB;

/* An indexed-colour image. Pixels are palette indexes (0-3 CGA, 0-15 VGA). */
typedef struct {
    int      w, h;
    uint8_t *px;
    int      ncolors;
    RGB      pal[16];
} Image;

typedef enum {
    PIC_TITLE,             /* CGA PM, VGA record 1 */
    PIC_TITLE_BARBARIAN,   /* PI, 2 */
    PIC_SCROLL_1,          /* PE, 3 */
    PIC_SCROLL_2,          /* PF, 4 */
    PIC_OPPONENTS,         /* PJ, 5 */
    PIC_SCENES_1,          /* PG, 6 */
    PIC_SCENES_2,          /* PH, 7 */
    PIC_OPTIONS_SHOPPE,    /* PK, 8 */
    PIC_REPORT_WIN,        /* PN, 9 */
    PIC_REPORT_LOSE,       /* PO, 10 */
    PIC_FORMATION_EDITOR,  /* PL, 11 */
    PIC_BATTLE_FIRST,      /* P0-PD, records 60-73 */
    PIC_COUNT = PIC_BATTLE_FIRST + NUM_BATTLE
} PictureId;

typedef enum { MODE_CGA, MODE_VGA, MODE_COUNT } GfxMode;

/* Squad poses on the campaign map (sprites at DS:031C-038C in WAR.EXE) */
typedef enum {
    POSE_MARCHING,   /* moving */
    POSE_STANDING,   /* no orders: how every squad starts */
    POSE_FIGHTING,
    POSE_WEAK,       /* condition 10 or less */
    POSE_SMALL,
    POSE_COUNT
} SquadPose;

typedef struct {
    bool  loaded;
    Image pictures[PIC_COUNT];
    Image tiles[NUM_TILES];
} AssetSet;

#define DS_SIZE    0xB40  /* initialised part of WAR.EXE's data segment */
#define RND_CODE   0x2330 /* resident code bytes read by WAR.EXE's random() */
#define O5_SIZE    0x373  /* O5: campaign-play messages */
#define OPP_PARAMS 19     /* words per commander in O2 */

typedef struct {
    char     dir[1024];
    uint8_t  font[FONT_COUNT][8];      /* 8x8, 1 bit per pixel, MSB = left */
    uint8_t  ds[DS_SIZE];              /* WAR.EXE data segment: strings, tables, icons */
    uint8_t  code[RND_CODE];           /* start of WAR.EXE's resident code */
    uint8_t  o5[O5_SIZE + 1];          /* O5, zero-terminated */
    bool     have_o5;
    uint8_t  o6[0x2000], o7[0x800], formations[420];   /* battle: sprites and scripts, F */
    bool     have_battle;
    int16_t  opponent[8][OPP_PARAMS];  /* each commander's values (O2 + 277h), loaded to DS:A4DC */
    bool     have_o2;
    uint8_t  squad_sprite[POSE_COUNT][SPRITE_H][SPRITE_W];   /* 1 = figure */
    uint8_t  flag_sprite[SPRITE_H][SPRITE_W];
    AssetSet set[MODE_COUNT];
} GameAssets;

extern const RGB CGA_PALETTE[4];       /* palette 1, high intensity */

const char *picture_name(PictureId id);
const char *mode_name(GfxMode mode);

/* Load everything from the folder holding the original game files.
 * The CGA set is required; the VGA set is loaded if WARET.DAT is present.
 * Returns false and fills err on failure. */
bool assets_load(GameAssets *ga, const char *dir, char *err, size_t errlen);
void assets_free(GameAssets *ga);

/* Load a file from the game folder, ignoring case. Free with SDL_free. */
void *assets_load_file(const char *dir, const char *name, size_t *size);

/* A zero-terminated string from WAR.EXE's data segment or from O5 ("" if out of range). */
const char *assets_ds_string(const GameAssets *ga, unsigned offset);
const char *assets_o5_string(const GameAssets *ga, unsigned offset);

/* 2bpp CGA bitmap from the data segment as draw_bitmap uses it: a width word
 * (in words), a height word (in line pairs), then the rows. Returns the pixel
 * value (0-3) at x,y, or 0 outside. */
int assets_ds_bitmap_px(const GameAssets *ga, unsigned offset, int x, int y, int *w, int *h);

/* True if dir looks like it holds the original game (has WAR.EXE). */
bool assets_dir_ok(const char *dir);

#endif
