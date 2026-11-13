/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * assets.h: loads the graphics from the player's own copy of the original game.
 *
 * CGA set:       WAS.EXE (font, map sprites), SM (map tiles), SP*, AD, B (pictures)
 * 16-colour set: WAST.DAT (the Tandy 1000 version's pictures and map tiles)
 */
#ifndef WAS_ASSETS_H
#define WAS_ASSETS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SCREEN_W    320
#define SCREEN_H    200
#define NUM_TILES   94
#define TILE_W      32
#define TILE_H      20
#define FONT_FIRST  0x20   /* first character in the font (space) */
#define FONT_COUNT  95     /* space .. '~' */
#define DS_SIZE     0x3F10 /* initialised part of WAS.EXE's data segment */
#define DS_FONT     0x089C /* the 8x8 font (the same as WAR.EXE's) */
#define RND_CODE    0x2330 /* resident code bytes read by WAS.EXE's random() (rt:2F0D) */
#define OPP_WORDS   25     /* an opponent's values: SO2 + 277h + 50 x opponent -> DS:604D */
#define SCNF_SIZE   29     /* SCNF: the Options Shoppe's settings -> DS:684C */

typedef struct { uint8_t r, g, b; } RGB;

/* An indexed-colour image. Pixels are palette indexes (0-3 CGA, 0-15 Tandy). */
typedef struct {
    int      w, h;
    uint8_t *px;
    int      ncolors;
    RGB      pal[16];
} Image;

typedef enum {
    PIC_TITLE,            /* SPM: "presented by", WAST.DAT record 1 */
    PIC_TITLE_SHIP,       /* SPI: the ship and its gunners, 2 */
    PIC_SCROLL_1,         /* SPE, 3 */
    PIC_SCROLL_2,         /* SPF, 4 */
    PIC_OPPONENTS,        /* SPJ, 5 */
    PIC_SCENES_1,         /* SPG, 6 */
    PIC_SCENES_2,         /* SPH, 7 */
    PIC_OPTIONS_SHOPPE,   /* SPK, 8 */
    PIC_REPORT_1,         /* SPN: battle report, cannon, 10 */
    PIC_REPORT_2,         /* SPO: battle report, boarding, 9 */
    PIC_REPORT_3,         /* SPP: CGA only */
    PIC_REPORT_4,         /* SPQ: CGA only */
    PIC_FACE,             /* AD: the opening's face, 13 */
    PIC_DECK,             /* B: boarding, the view along the deck, 29 */
    PIC_COUNT
} PictureId;

/* CGA: WAS.EXE. MODE_16: WAST.EXE, the 16-colour version for the Tandy 1000 */
typedef enum { MODE_CGA, MODE_16, MODE_COUNT } GfxMode;

/* Map sprites in WAS.EXE's data segment, drawn by rt_2941 (draw_map_sprite) */
typedef enum {
    MSPR_ANCHOR,          /* DS:049B: a stopped ship */
    MSPR_BATTLE,          /* DS:04AD: crossed swords, a squadron in an encounter (DS:56FF) */
    MSPR_HEADING_0,       /* DS:04BF-059D: a ship under sail, 16 headings (the table at DS:607F) */
    MSPR_PENNANT = MSPR_HEADING_0 + 16,   /* DS:05AF: drawn above a ship when [5EBD] = 1 */
    MSPR_CARGO,           /* DS:05BD: drawn above a ship when [5EBD] = 2 */
    MSPR_COUNT
} MapSprite;

#define MSPR_MAX_W 8      /* 2 bytes = 8 pixels */
#define MSPR_MAX_H 8      /* up to 4 line pairs */

typedef struct {
    int     w, h;                             /* pixels */
    uint8_t px[MSPR_MAX_H][MSPR_MAX_W];       /* 0-3 (only 0 and 3 are used) */
} Sprite;

typedef struct {
    bool  loaded;
    Image pictures[PIC_COUNT];   /* px NULL where the set has no such picture */
    Image tiles[NUM_TILES];
} AssetSet;

typedef struct {
    char     dir[1024];
    uint8_t  ds[DS_SIZE];                     /* WAS.EXE's data segment */
    uint8_t  font[FONT_COUNT][8];             /* 8x8, 1 bit per pixel, MSB = left */
    Sprite   map_sprite[MSPR_COUNT];
    int8_t   heading_dy[16];                  /* DS:075A: 0 or 1 line pair lower for each heading */
    uint8_t  so5[0x374];                      /* SO5: campaign-play messages (DS:4AF5), zero-terminated */
    bool     have_so5;
    uint8_t  code[RND_CODE];                  /* the start of WAS.EXE's resident code (random) */
    int16_t  opp[6][OPP_WORDS];               /* SO2: the six opponents' values */
    bool     have_so2;
    uint8_t  scnf[SCNF_SIZE];                 /* SCNF, or WAS.EXE's defaults (ovl1_startup) */
    /* the menus (step 6, overlay 1): SO2 (the opponents, the quotes, the
       report, the Options Shoppe, the opening), SO3 (the scrolls' letters and
       words), SO4 (the campaign menu), the WAS disk's SITLES and SDITLES */
    uint8_t  so2[0x1600], so3[0x1000], so4[0x200];
    uint8_t  sitles[876], sditles[876];
    bool     have_menus;
    AssetSet set[MODE_COUNT];
} GameAssets;

extern const RGB CGA_PALETTE[4];       /* palette 1, high intensity: every picture but the scrolls */
extern const RGB CGA_PALETTE_0[4];     /* palette 0, high intensity: the scrolls (show_scroll ovl1:7F1A) */
extern const RGB STD16_PALETTE[16];    /* the standard 16 colours (CGA/EGA/Tandy) */

const char *picture_name(PictureId id);
const char *picture_file(PictureId id);
const char *mode_name(GfxMode mode);

/* Load everything from the folder holding the original game files.
 * The CGA set is required; the 16-colour set is loaded if WAST.DAT is present.
 * Returns false and fills err on failure. */
bool assets_load(GameAssets *ga, const char *dir, char *err, size_t errlen);
void assets_free(GameAssets *ga);

/* Load a file from the game folder, ignoring case. Free with SDL_free. */
void *assets_load_file(const char *dir, const char *name, size_t *size);

/* The Options Shoppe's word at DS:offset (684C-6868), from SCNF */
int  assets_scnf_word(const GameAssets *ga, unsigned ds_offset);

/* A zero-terminated string from WAS.EXE's data segment or SO5 ("" if out of range). */
const char *assets_ds_string(const GameAssets *ga, unsigned offset);
const char *assets_so5_string(const GameAssets *ga, unsigned offset);

/* A zero-terminated string from a menu file ("" if out of range) */
const char *assets_buf_string(const uint8_t *buf, size_t size, unsigned offset);

/* True if dir looks like it holds the original game (has WAS.EXE). */
bool assets_dir_ok(const char *dir);

/* The picture decoders, also used by the tests:
 * load_picture (ovl1:CBEA): per field, word N then N codes; c <= 200 repeats
 * the next byte c times, c > 200 copies the next c - 200 bytes. Fills 16000
 * bytes (even lines, then odd lines). False if the data is short or wrong. */
bool assets_decode_picture(const uint8_t *data, size_t size, uint8_t fields[16000]);
/* WAST.DAT's run-length code: n >= 0 copies n+1 bytes, n < 0 repeats the next byte -n times. */
bool assets_unpack16(const uint8_t *src, size_t srclen, uint8_t *out, size_t size);

#endif
