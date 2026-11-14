/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * menu.c: step 6, the title pages, scrolls, menus, Options Shoppe, saved
 * games and the battle report, following overlay 1 of WAR.EXE.
 *
 * Overlay 1 draws straight into video memory: pictures, XOR boxes, an
 * AND-drawn pointer, text rows and icons. The menu keeps its own canvas the
 * same way, so what is left on the screen from one page shows until the next
 * covers it (as when a scroll unrolls over the title page). In CGA mode the
 * canvas keeps the 2-bit colour values, so a palette change recolours it as
 * on real hardware; in VGA mode it is remapped to the nearest colours.
 *
 * Each function names the WAR.EXE routine it follows.
 */
#include "menu.h"
#include "formed.h"
#include "mapedit.h"
#include "savegame.h"
#include "sound.h"
#include "sqedit.h"

#include <SDL3/SDL.h>

enum {
    MS_PRESENTED, MS_TITLE, MS_ASK_SAVED, MS_TITLES, MS_STORY, MS_RULES, MS_OPPONENT, MS_QUOTE,
    MS_REPORT, MS_CAMPAIGN_MENU, MS_SHOPPE, MS_SAVE_WARN, MS_SAVED, MS_MESSAGE, MS_DEMO,
    /* step 7: the campaign editor */
    MS_EDIT_MENU,             /* editor_menu (PH): rulebook, storybook, map maker, exit, enemy, army */
    MS_MAPEDIT,               /* overlay 4 */
    MS_SQEDIT,                /* overlay 3 */
    MS_EDIT_TEXT,             /* scroll_editor: the story, or a new title */
    MS_EDIT_ASK,              /* "Save the last changes? (Y or N)" after the story */
    MS_ERASE_ASK,             /* "Erase this title? (Y or N)" */
    MS_ERASE,                 /* the title is wiped off the scroll */
    MS_NOTE,                  /* a line of text for a moment, then the title page */
    MS_FORMED,                /* overlay 5: teach squads new formations */
};

enum { EDIT_NONE, EDIT_NEW, EDIT_CHANGE, EDIT_REMOVE };   /* campaign menu items 0, 1 and 2 */

/* The demo starts when the title page has been left alone for 189 turns of
 * its loop (ovl1:42C4). Each turn is two calibrated delays, so the wait
 * depended on the PC (DOSBox at 1500 cycles: 23 s; at 300 cycles, an IBM
 * PC/XT: about 12 s; a faster PC made it longer). The remake waits the 30
 * seconds the manual gives ("If you don't do anything for 30 seconds..."). */
#define DEMO_IDLE_FRAMES (30 * MENU_HZ)

#define MAX_DISKS 12              /* data disks the scroll can list */
#define DISKS_DIR "Data Disks"
#define DEMO_MOMENT 10            /* then "One moment, please" while the files load, */
#define DEMO_LOAD_FRAMES 30       /* then the map: 1 1/2 s in all, as in DOSBox */

#define SCROLL_LINES 400
#define VIEW_LINES   182          /* scroll window: the screen above the border and the two text rows */
#define TEXT_TOP     52           /* first text row on the scroll (BUF + 826h) */
#define TEXT_LEFT    24
#define ROW_STEP     12           /* 1E0h: 6 line pairs */

/* O3 (loaded where overlay 1 calls it O7, DS:A6D0) */
#define F_UPPER   0x000           /* 'A'-'Z': 16 x 10, 44 bytes each */
#define F_LOWER   0x478           /* 'a'-'z': 8 x 8, 20 bytes each (small capitals) */
#define F_QUEST   0x680
#define F_EXCL    0x694
#define F_SPACE   0x6A8           /* 8 x 10 of parchment: clears a cell */
#define F_COMMA   0x6C0
#define F_QUOTES  0x6CC
#define F_PERIOD  0x6D8
#define F_DIGITS  0x6E0           /* '0'-'9': 24 bytes each */
#define F_POINTER 0x8B4           /* the sword pointer, AND-drawn */
#define F_QUILL   0x7D0           /* the quill of the scroll editor, AND-drawn */
#define F_COLON   0xCF2
#define F_RPAREN  0xD06
#define F_LPAREN  0xD1A
#define F_DASH    0xD2E
#define F_SLASH   0xD42
#define F_UNDER   0xD56
#define F_BAR     0xD6E
#define I_ESC     0xCBE           /* key icons in O3 */
#define I_SPACEBAR 0xC36
#define I_ENTER   0x424           /* key icons in WAR.EXE's data segment */
#define I_PLUS    0x448
#define I_MINUS   0x46C

struct Menu {
    GameAssets *ga;
    Campaign   *camps;
    char        data_root[1024];  /* the data folder: CNF, F, DRIVE and Data Disks/ */
    char        data_dir[1100];   /* the data disk "in the drive": Data Disks/<name>/ */
    char        disks[MAX_DISKS][64];   /* the data disk folders, sorted */
    int         ndisks, disk;     /* how many, and which is in the drive */
    int         shoppe_disk;      /* the Options Shoppe's choice (ndisks: a new one) */
    bool        disk_list;        /* the scroll lists the data disks */
    GfxMode     mode;
    VideoCard   card;             /* the card it was drawn for (gfx_card) */
    uint8_t     tfg;              /* the text rows' colour: colour 3 on a CGA, 15 in WARET.EXE */
    bool        drawn;            /* canvas holds the current page in this mode */
    Screen      cv;               /* the canvas */
    int         npal;
    bool        pal0;             /* CGA palette 0 (the scrolls) */
    uint8_t     c4[4];            /* canvas index for each CGA colour value */
    uint8_t     inv[16];          /* XOR 0xFF of each canvas index */
    int         state, frame, step;
    MenuAction  action;
    bool        disk_error;       /* "Unable to use data disk." is up (ovl1_61fd) */
    bool        tune_started;     /* this page's tune has been started (not again when Tab redraws it) */
    uint32_t    seed;
    uint32_t    rnd;

    /* title page */
    int         title_item;
    bool        box_inverted;
    int         idle;             /* frames with no key ([bp-6] of ovl1_4161 counts turns) */

    /* scrolls */
    uint8_t     sbuf[SCREEN_W * SCROLL_LINES];
    int         a692;             /* which scroll (WAR.EXE's [A692]) */
    int         view;             /* top line of the window */
    int         row;              /* [A8C5]: text row of the pointer */
    uint8_t     text[840];        /* DS:9782: 26 rows of 32, 0x01 after capitals */
    int         anim;             /* 1 unrolling, 2 rolling up */
    int         anim_y;
    int         next_state;
    int         sel, count;       /* [A694], [A8CD] */
    bool        data_list;        /* the titles of the data folder */
    uint8_t     data_titles[876];
    int         rule;
    Campaign    game;

    /* opponents and the campaign menu */
    int         opp;
    int         opp_phase;
    int         sweep;            /* position of the sweeping box, -1 none */
    bool        frame_on;
    int         menu_item;
    bool        menu_first;

    /* Options Shoppe */
    MenuOptions opt;
    int         shoppe_item;
    bool        opt_changed;

    /* battle report */
    int         report_code, rep[6], rep_opp;

    /* saving */
    const Play *save_from;
    char        message[2][41];
    int         message_back;

    /* the campaign editor (step 7): m->game is the campaign being made or changed */
    int         edit;             /* EDIT_NEW, EDIT_CHANGE, EDIT_REMOVE */
    int         edit_item;        /* [A680]: the editor menu's scene */
    char        edit_file[4];     /* the campaign's T file on the data disk */
    int         ed_col, ed_row;   /* the quill: [A8C3] / 2, [A8C5] */
    int         ed_typed;         /* scroll_editor [bp-2]: letters typed (a title needs one) */
    bool        ed_wait;          /* the step's words are shown: any key to begin */
    int         ed_anim;          /* a new title: the quill goes down past the titles */
    bool        ask_yes;          /* the answer, shown for a moment */
    int         held;             /* the cursor key held down (the squads editor) */
    char        note[41];
    int         note_col, note_row;
    MapEdit     mapedit;
    SquadEdit   sqedit;
    FormEdit    formed;
};

/* ------------------------------------------------------------------ canvas */

static int rnd(Menu *m) { m->rnd = m->rnd * 1103515245u + 12345u; return (int)((m->rnd >> 16) & 0x7F); }

static uint8_t nearest(const RGB *pal, int n, RGB c)
{
    int best = 0, bd = 1 << 30;
    for (int i = 0; i < n; i++) {
        int dr = pal[i].r - c.r, dg = pal[i].g - c.g, db = pal[i].b - c.b, d = dr * dr + dg * dg + db * db;
        if (d < bd) { bd = d; best = i; }
    }
    return (uint8_t)best;
}

/* set_cga_palette / a picture's palette. VGA: the canvas is remapped. */
static void set_palette(Menu *m, const RGB *pal, int n, bool pal0)
{
    if (m->mode == MODE_VGA && m->npal > 0) {
        uint8_t map[16];
        for (int i = 0; i < 16; i++) map[i] = nearest(pal, n, m->cv.pal[i < m->npal ? i : 0]);
        for (int i = 0; i < SCREEN_W * SCREEN_H; i++) m->cv.px[i] = map[m->cv.px[i] & 15];
    }
    for (int i = 0; i < 16; i++) m->cv.pal[i] = pal[i < n ? i : 0];
    m->npal = n;
    m->pal0 = pal0;
    const RGB *cga = pal0 ? CGA_PALETTE_0 : CGA_PALETTE;
    for (int i = 0; i < 4; i++) m->c4[i] = m->mode == MODE_CGA ? (uint8_t)i : nearest(pal, n, cga[i]);
    m->tfg = m->mode == MODE_CGA ? m->c4[3] : 15;   /* WARET.EXE: white text rows, on the scrolls too (DOSBox-X) */
    for (int i = 0; i < 16; i++) {
        if (m->mode == MODE_CGA) m->inv[i] = (uint8_t)(i ^ 3);
        else m->inv[i] = (uint8_t)(i ^ 15);   /* WARET.EXE XORs the 4-bit pixels (DOSBox-X: the campaign menu's frame) */
    }
}

static const Image *picture(const Menu *m, PictureId id)
{
    const AssetSet *set = &m->ga->set[m->mode];
    if (!set->loaded || !set->pictures[id].px) set = &m->ga->set[MODE_CGA];
    return &set->pictures[id];
}

/* load_picture_rle8 + blit_buffer_to_cga */
static void show_picture(Menu *m, PictureId id, bool pal0)
{
    const Image *img = picture(m, id);
    if (m->mode == MODE_CGA) set_palette(m, pal0 ? CGA_PALETTE_0 : CGA_PALETTE, 4, pal0);
    else set_palette(m, img->pal, img->ncolors, pal0);
    screen_blit(&m->cv, img, 0, 0);
}

static void put(Menu *m, int x, int y, uint8_t v)
{
    if (x >= 0 && y >= 0 && x < SCREEN_W && y < SCREEN_H) m->cv.px[y * SCREEN_W + x] = v;
}

/* clear_text_row */
static void clear_row(Menu *m, int row) { screen_fill(&m->cv, 0, row * 8, SCREEN_W, 8, m->c4[0]); }

/* draw_text_area_border (rt:1CEB, from text_rows_add): with n text rows at the
 * bottom, the picture ends at line 198 - 8n, under it a line in colour 3 and a
 * black line (n = 2: lines 182 and 183; n = 1: 190 and 191) */
static void text_border(Menu *m, int rows)
{
    int y = 198 - 8 * rows;
    screen_fill(&m->cv, 0, y, SCREEN_W, 1, m->tfg);
    screen_fill(&m->cv, 0, y + 1, SCREEN_W, 1, m->c4[0]);
}

/* print_string at a text position; \n goes down a row, \r to column 0 */
static void print_at(Menu *m, int col, int row, const char *s)
{
    char one[2] = { 0, 0 };
    for (; *s; s++) {
        if (*s == '\n') { row++; continue; }
        if (*s == '\r') { col = 0; continue; }
        if ((unsigned char)*s < 0x20) continue;
        if (col >= 40) { col = 0; row++; }
        if (row > 24) break;
        one[0] = *s;
        screen_text(&m->cv, m->ga, col * 8, row * 8, one, m->tfg, m->c4[0]);
        col++;
    }
}

static const char *O2S(const Menu *m, unsigned off) { return assets_buf_string(m->ga->o2, sizeof m->ga->o2, off); }
static const char *O3S(const Menu *m, unsigned off) { return assets_buf_string(m->ga->o3, sizeof m->ga->o3, off); }
static const char *O4S(const Menu *m, unsigned off) { return assets_buf_string(m->ga->o4, sizeof m->ga->o4, off); }
static const char *DSS(const Menu *m, unsigned off) { return assets_ds_string(m->ga, off); }

/* draw_bitmap: an icon from the data segment (ds) or O3, x in bytes */
static void icon(Menu *m, bool ds, unsigned off, int xb, int y)
{
    int w = 0, h = 0;
    const uint8_t *buf = ds ? m->ga->ds : m->ga->o3;
    size_t size = ds ? DS_SIZE : sizeof m->ga->o3;
    assets_bitmap_px(buf, size, off, 0, 0, &w, &h);
    for (int yy = 0; yy < h; yy++)
        for (int xx = 0; xx < w; xx++) {
            int v = assets_bitmap_px(buf, size, off, xx, yy, NULL, NULL);
            put(m, xb * 4 + xx, y + yy, v == 3 && y + yy >= VIEW_LINES ? m->tfg : m->c4[v]);   /* in the text rows: their colour */
        }
}

/* ovl1_7ffc: an O3 glyph copied into the canvas or the scroll (x in pixels) */
static void glyph(Menu *m, uint8_t *dst, int lines, unsigned off, int x, int y)
{
    int w = 0, h = 0;
    const uint8_t *o3 = m->ga->o3;
    assets_bitmap_px(o3, sizeof m->ga->o3, off, 0, 0, &w, &h);
    for (int yy = 0; yy < h; yy++) {
        if (y + yy < 0 || y + yy >= lines) continue;
        for (int xx = 0; xx < w; xx++) {
            if (x + xx < 0 || x + xx >= SCREEN_W) continue;
            dst[(y + yy) * SCREEN_W + x + xx] = m->c4[assets_bitmap_px(o3, sizeof m->ga->o3, off, xx, yy, NULL, NULL)];
        }
    }
}

/* ovl1_8037: the pointer (or the quill), ANDed onto the screen (x in bytes) */
static void and_bitmap(Menu *m, unsigned off, int xb, int y)
{
    int w = 0, h = 0;
    assets_bitmap_px(m->ga->o3, sizeof m->ga->o3, off, 0, 0, &w, &h);
    for (int yy = 0; yy < h; yy++)
        for (int xx = 0; xx < w; xx++) {
            int v = assets_bitmap_px(m->ga->o3, sizeof m->ga->o3, off, xx, yy, NULL, NULL);
            int px = xb * 4 + xx, py = y + yy;
            if (px < 0 || py < 0 || px >= SCREEN_W || py >= VIEW_LINES) continue;
            uint8_t *d = &m->cv.px[py * SCREEN_W + px];
            if (m->mode == MODE_CGA) *d = (uint8_t)(*d & v);
            /* WARET.EXE (DOSBox-X): a Tandy 1000 ANDs each 2-bit pixel as a 4-bit one
             * (01 -> 0011, 10 -> 1100); an EGA or a VGA draws the pixels with bit 0
             * clear in colour 1 (a blue pointer) and leaves the others */
            else if (gfx_card == CARD_TANDY) *d = (uint8_t)(*d & (((v & 2) ? 12 : 0) | ((v & 1) ? 3 : 0)));
            else if (!(v & 1)) *d = 1;
        }
}

static void pointer(Menu *m, int xb, int y) { and_bitmap(m, F_POINTER, xb, y); }

/* ovl1_8070: XOR a box with rounded corners (x, w in bytes; h in line pairs) */
static void xor_box(Menu *m, int xb, int y, int wb, int h)
{
    int x0 = xb * 4, x1 = (xb + wb + 2) * 4;            /* first and last bytes take part */
    int lines = 2 + 2 * h + 2;
    for (int l = 0; l < lines; l++) {
        int inset = (l == 0 || l == lines - 1) ? 2 : (l == 1 || l == lines - 2) ? 1 : 0;
        for (int x = x0 + inset; x < x1 - inset; x++) {
            int py = y + l;
            if (x < 0 || x >= SCREEN_W || py < 0 || py >= SCREEN_H) continue;
            m->cv.px[py * SCREEN_W + x] = m->inv[m->cv.px[py * SCREEN_W + x] & 15];
        }
    }
}

/* ovl1_810B: XOR a frame 4 pixels wide around a picture (x, w in bytes; h in line pairs) */
static void xor_frame(Menu *m, int xb, int y, int wb, int h)
{
    int x0 = xb * 4, w = wb * 4, lines = 4 + 2 * h + 4;
    for (int l = 0; l < lines; l++)
        for (int x = 0; x < w; x++) {
            bool edge = l < 4 || l >= lines - 4 || x < 4 || x >= w - 4;
            int px = x0 + x, py = y + l;
            if (!edge || px >= SCREEN_W || py >= SCREEN_H) continue;
            m->cv.px[py * SCREEN_W + px] = m->inv[m->cv.px[py * SCREEN_W + px] & 15];
        }
}

/* ovl1_6DE4: a lowercase string in the small capitals, on a picture (x in bytes) */
static void small_caps(Menu *m, int xb, int y, const char *s)
{
    for (; *s; s++, xb += 2) {
        glyph(m, m->cv.px, SCREEN_H, F_SPACE, xb * 4, y);
        int c = (unsigned char)*s;
        if (c == ' ') continue;
        if (c >= 'A' && c <= 'Z') c += 'a' - 'A';   /* the remake: names typed in capitals */
        unsigned off = c == '/' ? F_SLASH : c == ':' ? F_COLON : (c >= 'a' && c <= 'z') ? F_LOWER + (c - 'a') * 20 :
                       (c >= '0' && c <= '9') ? F_DIGITS + (c - '0') * 24 : c == '.' ? F_PERIOD : c == ',' ? F_COMMA :
                       c == '?' ? F_QUEST : c == '!' ? F_EXCL : c == '\'' ? F_QUOTES : F_SPACE;
        glyph(m, m->cv.px, SCREEN_H, off, xb * 4, y);
    }
}

/* ovl1_76DA + 4D08 and friends: the key help on rows 23 and 24 */
static void help_change_select(Menu *m, const char *change, int enter_x, int tail_col, const char *tail)
{
    clear_row(m, 24);
    icon(m, true, I_PLUS, 0, 192);
    print_at(m, 3, 24, DSS(m, 0x11C));          /* or */
    icon(m, true, I_MINUS, 12, 192);
    print_at(m, 9, 24, change);
    icon(m, true, I_ENTER, enter_x, 192);
    print_at(m, tail_col, 24, tail);
}

/* ------------------------------------------------------------------ the scroll */

static bool is_upper(int c) { return c >= 'A' && c <= 'Z'; }

/* ovl1_70C7: one character cell of the scroll text */
static void scroll_char(Menu *m, int col, int row, int c, bool old_upper)
{
    int x = TEXT_LEFT + col * 8, y = TEXT_TOP + row * ROW_STEP;
    glyph(m, m->sbuf, SCROLL_LINES, F_SPACE, x, y);
    if (old_upper) glyph(m, m->sbuf, SCROLL_LINES, F_SPACE, x + 8, y);
    unsigned off = F_SPACE;
    int dy = 0;
    if (is_upper(c))              off = F_UPPER + (c - 'A') * 44;
    else if (c >= 'a' && c <= 'z') { off = F_LOWER + (c - 'a') * 20; dy = 2; }
    else if (c >= '0' && c <= '9') off = F_DIGITS + (c - '0') * 24;
    else if (c == '|')            off = F_BAR;
    else if (c == '_')            off = F_UNDER;
    else if (c == '?')            { off = F_QUEST; dy = 2; }
    else if (c == '!')            { off = F_EXCL; dy = 2; }
    else if (c == '.')            { off = F_PERIOD; dy = 8; }
    else if (c == ',')            { off = F_COMMA; dy = 6; }
    else if (c == '\'')           off = F_COMMA;
    else if (c == '"')            off = F_QUOTES;
    else if (c == ':' || c == ';') { off = F_COLON; dy = 2; }
    else if (c == '(' || c == '[' || c == '{') { off = F_LPAREN; dy = 2; }
    else if (c == ')' || c == ']' || c == '}') { off = F_RPAREN; dy = 2; }
    else if (c == '-')            { off = F_DASH; dy = 2; }
    else if (c == '/')            { off = F_SLASH; dy = 2; }
    glyph(m, m->sbuf, SCROLL_LINES, off, x, y + dy);
}

/* ovl1_74D1: write the whole text onto the scroll; capitals take two cells */
static void scroll_text(Menu *m)
{
    for (int row = 0; row < 26; row++) {
        for (int col = 0; col < 31; col++) {
            int c = m->text[row * 32 + col];
            if (c == 1) continue;               /* the second cell of a capital */
            scroll_char(m, col, row, c, is_upper(c));
        }
    }
}

/* PE above PF: the parchment, 320 x 400 */
static void scroll_build(Menu *m)
{
    const Image *pe = picture(m, PIC_SCROLL_1), *pf = picture(m, PIC_SCROLL_2);
    uint8_t map[16];
    for (int i = 0; i < 16; i++) map[i] = (uint8_t)i;
    if (m->mode == MODE_VGA)
        for (int i = 0; i < pf->ncolors; i++) map[i] = nearest(m->cv.pal, m->npal, pf->pal[i]);
    for (int y = 0; y < 200; y++)
        for (int x = 0; x < SCREEN_W; x++) {
            m->sbuf[y * SCREEN_W + x] = (y < pe->h && x < pe->w) ? pe->px[y * pe->w + x] : m->c4[3];
            m->sbuf[(200 + y) * SCREEN_W + x] = (y < pf->h && x < pf->w) ? map[pf->px[y * pf->w + x] & 15] : m->c4[3];
        }
    scroll_text(m);
}

/* ovl1_7FBD: copy lines of the scroll to the screen */
static void scroll_copy(Menu *m, int from, int to, int lines)
{
    for (int l = 0; l < lines; l++) {
        if (to + l < 0 || to + l >= VIEW_LINES || from + l < 0 || from + l >= SCROLL_LINES) continue;
        SDL_memcpy(m->cv.px + (to + l) * SCREEN_W, m->sbuf + (from + l) * SCREEN_W, SCREEN_W);
    }
}

static void scroll_show(Menu *m)    /* blit_buffer_to_cga, and the pointer */
{
    scroll_copy(m, m->view, 0, VIEW_LINES);
    text_border(m, 2);
    int a = m->a692;
    if (m->ed_wait && (a == 0 || a == 11)) return;   /* the step's words first, then the pointer */
    if (a == 1 || a == 2 || a == 5 || a == 6 || a == 7 || a == 11)
        pointer(m, 3, (m->row + 4) * ROW_STEP + 4 - m->view);
    else if (m->state == MS_EDIT_TEXT && (a == 0 || a == 3 || a == 10) && !m->anim)   /* ovl1:6B1A */
        and_bitmap(m, F_QUILL, m->ed_col * 2 + (m->ed_anim ? 5 : 7), m->ed_row * ROW_STEP + 0x3A - m->view);
}

/* ovl1_7499: palette 0, then the top and the bottom curl of the scroll */
/* the scroll's colours (CGA palette 0, or PE's palette). Called before a
 * scroll page prints its help rows: the text colours (c4) come from the
 * palette, and after Tab the old mode's colours would print the rows black. */
static void scroll_palette(Menu *m)
{
    const Image *pe = picture(m, PIC_SCROLL_1);
    if (m->mode == MODE_CGA) set_palette(m, CGA_PALETTE_0, 4, true);
    else set_palette(m, pe->pal, pe->ncolors, true);
}

static void scroll_open(Menu *m)
{
    scroll_palette(m);
    text_border(m, 2);
    m->view = 0;
    scroll_build(m);
    scroll_copy(m, 0, 0, 36);
    scroll_copy(m, 370, 36, 28);
    m->anim = 1;                        /* ovl1_75C5: unroll */
    m->anim_y = 36;
}

/* one step of unrolling (ovl1_75C5) or rolling up (ovl1_7659) */
static bool scroll_anim(Menu *m)
{
    for (int k = 0; k < 4 && m->anim; k++) {
        /* the speaker hums at 40 Hz and is switched on or off at random each
         * step (ovl1:75E9, 7677), which makes the scroll crackle */
        snd_scroll_step(rnd(m) < (m->anim == 1 ? 95 : 80), 1.0 / (4 * MENU_HZ));
        if (m->anim == 1) {
            scroll_copy(m, m->anim_y, m->anim_y, 2);
            int h = SDL_min((182 - m->anim_y) >> 1, 14) * 2;
            scroll_copy(m, 370, m->anim_y + 2, h);
            m->anim_y += 2;
            if (m->anim_y >= 180) { m->anim = 0; snd_scroll_end(); scroll_show(m); }
        } else {
            int h = SDL_min((182 - m->anim_y) >> 1, 14) * 2;
            scroll_copy(m, 370, m->anim_y + 2, h);
            m->anim_y -= 2;
            if (m->anim_y <= 32) { m->anim = 0; snd_scroll_end(); scroll_copy(m, 0, 0, 36); return true; }
        }
    }
    return false;
}

static void scroll_close(Menu *m, int next)
{
    m->anim = 2;
    m->anim_y = 180;
    m->next_state = next;
    clear_row(m, 22);
}

/* ovl1_6D7E-like: write a string (with its 0x01 markers) into the text */
static void text_put(Menu *m, int col, int row, const char *s)
{
    for (; *s && col < 32; s++, col++) {
        m->text[row * 32 + col] = (uint8_t)*s;
        if (is_upper((unsigned char)*s) && col < 31) m->text[row * 32 + ++col] = 1;   /* a capital takes two cells */
    }
}

/* ovl1_5030: the rules, as sentences (5073: "Villages don't supply food", ...) */
static void rules_text(Menu *m)
{
    const uint16_t *r = m->game.rules;
    SDL_memset(m->text, ' ', sizeof m->text);
    char line[64];
    text_put(m, 10, 0, O3S(m, 0xA09));                                     /* RULES */
    SDL_snprintf(line, sizeof line, "%s%s%s", O3S(m, 0xA0F), r[RULE_VILLAGES_SUPPLY_FOOD] ? "" : O3S(m, 0xA86), O3S(m, 0xA8D));
    text_put(m, 2, 3, line);
    SDL_snprintf(line, sizeof line, "%s%s%s", O3S(m, 0xA19), r[RULE_FORTS_SUPPLY_FOOD] ? "" : O3S(m, 0xA86), O3S(m, 0xA8D));
    text_put(m, 2, 5, line);
    int v = r[RULE_SUPPLY_LINE_RANGE];
    SDL_snprintf(line, sizeof line, "%s%s", O3S(m, 0xA20), O3S(m, v == 1 ? 0xAA0 : v == 2 ? 0xAA6 : 0xAAD));
    text_put(m, 2, 7, line);
    v = r[RULE_FORTS_TRAIN_MEN];
    SDL_snprintf(line, sizeof line, "%s%s", O3S(m, 0xA36), O3S(m, v == 30 ? 0xAB4 : v == 60 ? 0xABF : 0xAC6));
    text_put(m, 2, 9, line);
    v = r[RULE_MEN_SHAPE];
    SDL_snprintf(line, sizeof line, "%s%s%s", O3S(m, 0xA47), O3S(m, v == 4 ? 0xAD1 : v == 2 ? 0xAD8 : 0xAE0), O3S(m, 0xAE9));
    text_put(m, 2, 11, line);
    v = r[RULE_ENEMY_SEEN];
    SDL_snprintf(line, sizeof line, "%s%s", O3S(m, 0xA55), O3S(m, v == 20 ? 0xAF0 : v == 40 ? 0xAFB : 0xB06));
    text_put(m, 2, 13, line);
    v = r[RULE_WATER];
    SDL_snprintf(line, sizeof line, "%s%s", O3S(m, 0xA64), O3S(m, v == 10 ? 0xB0F : v == 20 ? 0xB22 : 0xB33));
    text_put(m, 2, 15, line);
    v = r[RULE_MOUNTAIN];
    SDL_snprintf(line, sizeof line, "%s%s", O3S(m, 0xA6E), O3S(m, v == 4 ? 0xB46 : v == 8 ? 0xB55 : 0xB63));
    text_put(m, 2, 17, line);
    v = r[RULE_FOREST];
    SDL_snprintf(line, sizeof line, "%s%s", O3S(m, 0xA7B), O3S(m, v == 3 ? 0xB72 : 0xB79));
    text_put(m, 2, 19, line);
}

/* ovl1_6663: space changes the rule under the pointer */
static void rule_change(Menu *m, int k)
{
    uint16_t *r = m->game.rules;
    switch (k) {
    case 0: r[RULE_VILLAGES_SUPPLY_FOOD] = r[RULE_VILLAGES_SUPPLY_FOOD] ? 0 : 1; break;
    case 1: r[RULE_FORTS_SUPPLY_FOOD] = r[RULE_FORTS_SUPPLY_FOOD] ? 0 : 1; break;
    case 2: r[2] = (r[2] && (r[2] << 1) <= 4) ? r[2] << 1 : 1; break;
    case 3: r[3] = (r[3] && (r[3] << 1) <= 120) ? r[3] << 1 : 30; break;
    case 4: r[4] = (r[4] <= 4 && (r[4] >> 1)) ? r[4] >> 1 : 4; break;
    case 5: r[5] = (r[5] && (r[5] << 1) <= 80) ? r[5] << 1 : 20; break;
    case 6: r[6] = (r[6] && (r[6] << 1) <= 40) ? r[6] << 1 : 10; break;
    case 7: r[7] = (r[7] && (r[7] << 1) <= 16) ? r[7] << 1 : 4; break;
    default: r[8] = (r[8] && (r[8] << 1) <= 6) ? r[8] << 1 : 3; break;
    }
}

/* ------------------------------------------------------------------ pages */

static void page_title(Menu *m);
static void page_titles(Menu *m, bool unroll);
static void page_story(Menu *m);
static void page_rules(Menu *m);
static void page_opponent(Menu *m);
static void page_campaign_menu(Menu *m);
static void page_shoppe(Menu *m);
static void page_report(Menu *m);

/* WARET.EXE on an EGA or a VGA puts a few texts one byte (4 pixels) further
 * left than WAR.EXE and its own Tandy version do: the title page's item and
 * your column of the battle report (DOSBox-X) */
static int ega_left(const Menu *m) { return m->mode != MODE_CGA && gfx_card != CARD_TANDY; }

static void page_edit_text(Menu *m, bool unroll);

static void go(Menu *m, int state) { m->state = state; m->frame = 0; m->step = 0; m->drawn = false; m->tune_started = false; }

/* a page's tune, once: a redraw after Tab or F2 doesn't start it again */
static bool tune_once(Menu *m)
{
    if (m->tune_started) return false;
    m->tune_started = true;
    return true;
}

/* ovl1_4161 show_title_barbarian: the item box on PI */
static const unsigned TITLE_ITEMS[3] = { 0x951, 0x95E, 0x96B };    /* go to war, make changes, quit */

static void title_box(Menu *m)
{
    if (m->box_inverted) xor_box(m, 17, 54, 25, 5);   /* the buffer is blitted again: back to normal */
    m->box_inverted = false;
    small_caps(m, 19 - ega_left(m), 58, O3S(m, TITLE_ITEMS[m->title_item]));   /* WARET.EXE on an EGA or VGA: 4 pixels left */
    m->frame = 0;
    m->idle = 0;
}

/* ovl1:42CC: nobody has pressed a key for a while: "demo" in the box, and one
 * of the campaigns T3-TA, against the first opponent, every squad seen. */
static void demo_start(Menu *m)
{
    int n = 3 + (rnd(m) & 7);
    char name[3] = { 'T', (char)(n < 10 ? '0' + n : 'A' + n - 10), 0 };
    int idx = -1;
    for (int c = 0; c < NUM_CAMPAIGNS; c++)
        if (m->camps[c].loaded && SDL_strcasecmp(m->camps[c].file, name) == 0) idx = c;
    if (idx < 0 || !m->ga->have_battle) { m->idle = 0; return; }   /* not on this WAR disk: keep waiting */
    small_caps(m, 19 - ega_left(m), 58, O3S(m, 0x96B));   /* "    quit    " */
    small_caps(m, 27 - ega_left(m), 58, O3S(m, 0xE54));   /* "demo " over "quit" */
    clear_row(m, 24);
    m->game = m->camps[idx];
    m->game.opponent = 0;                       /* A67C = 0, its values from O2 */
    m->game.rules[RULE_ENEMY_SEEN] = 80;        /* A70F: every squad is seen */
    m->seed = (uint32_t)SDL_GetTicksNS();
    m->state = MS_DEMO;                         /* the picture stays while the files load */
    m->frame = 0;
}

static void page_title(Menu *m)
{
    show_picture(m, PIC_TITLE_BARBARIAN, false);
    text_border(m, 1);                  /* [A5C0] = 1: the picture down to line 189 (ovl1:41A0) */
    help_change_select(m, O3S(m, 0x991), 40, 23, O3S(m, 0xBF6));
    m->box_inverted = false;            /* a fresh picture */
    title_box(m);
}

/* ovl1_30F0 startup: "presented by" */
static void page_presented(Menu *m)
{
    show_picture(m, PIC_TITLE, false);
    print_at(m, 13, 12, DSS(m, 0x4C));
    if (tune_once(m)) snd_tune(m->ga->o2, 0x447, 0x48B, 0x110D, 0x1157, 0x119F, 0x11E7);   /* ovl1:31FE: the opening tune */
}

/* the TITLES scroll (ovl1_62FA) */
static void titles_count(Menu *m)
{
    m->count = 0;                                /* ovl1_74D1: rows after the heading with text at column 2 */
    for (int r = 2; r < 26; r++) {
        const uint8_t *t = &m->text[r * 32 + 2];
        if (t[0] != ' ' || t[1] != ' ') m->count++;
    }
    m->count--;
}

static void page_titles(Menu *m, bool unroll)
{
    scroll_palette(m);
    bool editing = m->edit == EDIT_CHANGE || m->edit == EDIT_REMOVE;
    const uint8_t *src = (m->data_list || editing) ? m->data_titles : m->ga->titles;
    SDL_memcpy(m->text, src + 36, 840);
    if (m->disk_list) {                          /* the remake: the data disks, laid out like TITLES */
        SDL_memset(m->text, ' ', 840);
        text_put(m, 7, 0, "DATA DISKS");
        for (int i = 0; i < m->ndisks; i++) {
            char name[32];
            SDL_strlcpy(name, m->disks[i], sizeof name);
            text_put(m, 2, 3 + 2 * i, name);
        }
    }
    m->a692 = m->edit == EDIT_CHANGE ? 7 : m->edit == EDIT_REMOVE ? 5 : 1;
    titles_count(m);
    m->row = 3 + 2 * m->sel;
    m->view = 0;
    while (TEXT_TOP + m->row * ROW_STEP - m->view > 142) m->view += ROW_STEP;
    for (int r = 23; r < 25; r++) clear_row(m, r);
    /* ovl1_655A: Select a campaign title.  [Esc] to back up / [+] or [-] to change, [Enter] to select. */
    if (m->a692 == 5) {                          /* Changed your mind?  Press [Esc] / [+] or [-] to select, [Enter] to erase. */
        print_at(m, 0, 23, O3S(m, 0xB9B));
        icon(m, false, I_ESC, 52, 184);
        help_change_select(m, O3S(m, 0xBF6), 40, 23, O3S(m, 0xCB4));
    } else if (m->a692 == 7) {                   /* Select the campaign you want to change. / ... to point, ... */
        print_at(m, 0, 23, O3S(m, 0xBB5));
        help_change_select(m, O3S(m, 0x991), 40, 23, O3S(m, 0xBF6));
        print_at(m, 12, 24, O3S(m, 0xEDE));
    } else {
        icon(m, false, I_ESC, 52, 184);
        print_at(m, 30, 23, DSS(m, 0x10A));
        print_at(m, 0, 23, m->disk_list ? "Select a data disk.     " : O3S(m, 0xBDD));
        help_change_select(m, O3S(m, 0x991), 40, 23, O3S(m, 0xBF6));
    }
    if (unroll) scroll_open(m);
    else {
        scroll_palette(m);
        scroll_build(m);
        scroll_show(m);
    }
}

static void page_story(Menu *m)
{
    scroll_palette(m);
    SDL_memcpy(m->text, m->game.story_raw, 840);
    m->a692 = 4;
    for (int r = 23; r < 25; r++) clear_row(m, r);
    print_at(m, 0, 23, O3S(m, 0x90A));                 /* This is the story of this campaign. */
    clear_row(m, 24);
    icon(m, true, I_PLUS, 0, 192);
    print_at(m, 3, 24, DSS(m, 0x11C));
    icon(m, true, I_MINUS, 12, 192);
    print_at(m, 9, 24, O3S(m, 0x92E));                 /* to scroll, */
    icon(m, true, I_ENTER, 39, 192);
    print_at(m, 23, 24, O3S(m, 0x939));                /* when done. */
    scroll_open(m);
}

static void rules_help(Menu *m)            /* ovl1:66EE */
{
    for (int r = 23; r < 25; r++) clear_row(m, r);
    icon(m, true, I_PLUS, 0, 184);
    print_at(m, 3, 23, DSS(m, 0x115));
    icon(m, true, I_MINUS, 12, 184);
    print_at(m, 9, 23, DSS(m, 0x118));
    print_at(m, 12, 23, O3S(m, m->a692 == 2 ? 0xC01 : 0xC14));   /* look at the rules. / select the rule to change. */
    icon(m, false, I_SPACEBAR, 0, 192);
    print_at(m, 7, 24, O3S(m, 0x991));                 /* to change, */
    icon(m, true, I_ENTER, 37, 192);
    print_at(m, 22, 24, O3S(m, 0x939));                /* when done. */
}

static void page_rules(Menu *m)
{
    scroll_palette(m);
    m->a692 = m->edit == EDIT_NEW ? 11 : m->edit == EDIT_CHANGE ? 6 : 2;
    rules_text(m);
    m->rule = 0;
    m->row = 3;
    if (m->a692 == 11 && m->ed_wait) {                 /* ovl1:66A4: The fourth step is to set the rules. Press any key. */
        for (int r = 23; r < 25; r++) clear_row(m, r);
        char line[64];
        SDL_snprintf(line, sizeof line, "%s%s", O3S(m, 0xD96), O3S(m, 0xED0));
        print_at(m, 0, 23, line);
        SDL_snprintf(line, sizeof line, "%s%s", O3S(m, 0x9E5), O3S(m, 0xEC6));
        print_at(m, 0, 24, line);
    } else {
        rules_help(m);
    }
    scroll_open(m);
}

/* choose_opponent (ovl1_4808): PJ */
static int o2w(const Menu *m, unsigned off) { return (int16_t)(m->ga->o2[off] | (m->ga->o2[off + 1] << 8)); }

static void opponent_frame(Menu *m)
{
    int pos = o2w(m, 0x247 + 2 * m->opp);
    xor_frame(m, pos % 80, (pos / 80) * 2, o2w(m, 0x257 + 2 * m->opp), o2w(m, 0x267 + 2 * m->opp));
    m->frame_on = !m->frame_on;
}

static void opponent_text(Menu *m)
{
    static const unsigned DESC[8] = { 0, 68, 144, 222, 0x121, 0x169, 0x1B7, 0x200 };   /* DS:A682 */
    clear_row(m, 0);
    clear_row(m, 1);
    print_at(m, 0, 0, O2S(m, DESC[m->opp]));
}

static void page_opponent(Menu *m)
{
    show_picture(m, PIC_OPPONENTS, false);
    m->frame_on = false;
    m->sweep = -1;
    opponent_text(m);
    clear_row(m, 24);
    if (m->opp_phase == 2) {                           /* ovl1:496B: a new campaign's third step */
        clear_row(m, 0); clear_row(m, 1);
        print_at(m, 0, 0, O2S(m, 0x3A7));             /* The third step is to select the enemy */
        char line[64];
        SDL_snprintf(line, sizeof line, "%s %s%s", O2S(m, 0x3CD), O3S(m, 0x9E5), O2S(m, 0x3D5));
        print_at(m, 0, 1, line);                      /* leader. Press any key. */
    } else if (m->opp_phase == 0) {
        print_at(m, 0, 24, O2S(m, 0x3DF));             /* [+]to change the opponent, [Enter]to continue */
        icon(m, true, I_PLUS, 0, 192);
        icon(m, true, I_ENTER, 51, 192);
    } else {
        help_change_select(m, O3S(m, 0x991), 40, 23, O3S(m, 0xBF6));
    }
}

/* the quote before the war (show_title_barbarian, ovl1_447B) */
static void page_quote(Menu *m)
{
    clear_row(m, 24);
    print_at(m, 11, 24, O3S(m, 0x978));                /* PREPARE FOR WAR! */
    int k;
    do k = rnd(m); while (k > 38);
    clear_row(m, 12);
    clear_row(m, 11);
    print_at(m, 0, 11, O2S(m, (unsigned)o2w(m, 0x4CD + 2 * k)));
    clear_row(m, 10);
    clear_row(m, 13);
    screen_fill(&m->cv, 0, 80, SCREEN_W, 1, m->c4[3]);
    screen_fill(&m->cv, 0, 112, SCREEN_W, 1, m->c4[3]);
}

/* campaign_menu (ovl1_3D1E): PG and O4 */
static int o4w(const Menu *m, unsigned off) { return (int16_t)(m->ga->o4[off] | (m->ga->o4[off + 1] << 8)); }

/* the campaign menu (PG, tables at O4+19Bh) and the editor menu (PH, O4+1BFh) */
static bool editor_menu(const Menu *m) { return m->state == MS_EDIT_MENU; }
static int *scene_item(Menu *m) { return editor_menu(m) ? &m->edit_item : &m->menu_item; }

static void menu_frame_box(Menu *m)
{
    unsigned t = editor_menu(m) ? 0x1BF : 0x19B;
    int k = *scene_item(m), pos = o4w(m, t + 2 * k);
    xor_frame(m, pos % 80, (pos / 80) * 2, o4w(m, t + 12 + 2 * k), o4w(m, t + 24 + 2 * k));
    m->frame_on = !m->frame_on;
}

static void scene_text(Menu *m)
{
    clear_row(m, 0); clear_row(m, 1);
    print_at(m, 15, 0, O4S(m, (unsigned)o4w(m, (editor_menu(m) ? 0x1EF : 0x1E3) + 2 * *scene_item(m))));
}

static void page_campaign_menu(Menu *m)
{
    show_picture(m, editor_menu(m) ? PIC_SCENES_2 : PIC_SCENES_1, false);
    m->frame_on = false;
    m->sweep = -1;
    scene_text(m);
    /* campaign_menu: PG shows down to line 191. WARET.EXE leaves row 23 alone: its picture has a line
     * on line 187 (DOSBox-X, EGA and Tandy) */
    if (editor_menu(m)) for (int r = m->mode == MODE_CGA ? 23 : 24; r < 25; r++) clear_row(m, r);
    help_change_select(m, O3S(m, 0xD86), 50, 28, O3S(m, 0xBF6));   /* to move marker, [Enter] to select. */
}

/* options_shoppe (ovl1_5782): PK */
static const int SHOPPE_Y[6] = { 10, 22, 34, 46, 58, 70 };   /* 19Ch, 37Ch, 55Ch, 73Ch, 91Ch, AFCh */

static void shoppe_line(Menu *m, int k)
{
    char s[48];
    const MenuOptions *o = &m->opt;
    switch (k) {
    case 0:
        if (m->mode != MODE_CGA)    /* WARET.EXE (ovl 0x9404): "ega", "vga" or "tandy", ": sixteen colors" */
            SDL_snprintf(s, sizeof s, "%s: sixteen colors", gfx_card == CARD_TANDY ? "tandy" : gfx_card == CARD_EGA ? "ega" : "vga");
        else SDL_snprintf(s, sizeof s, "%s%s", O2S(m, 0x1375), O2S(m, o->display == 0 ? 0x1381 : o->display == 1 ? 0x138E : 0x139B));
        break;
    case 1: SDL_snprintf(s, sizeof s, "%s%s", O2S(m, 0x13DE), O3S(m, o->fight_delay == 200 ? 0xAA0 : o->fight_delay == 400 ? 0xAA6 : 0xAAD)); break;
    case 2:                     /* the remake: which data disk folder is in the drive, not A: or B: */
        if (m->shoppe_disk >= m->ndisks) SDL_strlcpy(s, "a new data disk", sizeof s);
        else SDL_snprintf(s, sizeof s, "disk %.18s", m->disks[m->shoppe_disk]);   /* 23 letters fit beside the arm */
        break;
    case 3:
        if (o->sounds == 0)      SDL_snprintf(s, sizeof s, "%s%s", O2S(m, 0x13A8), O2S(m, 0x13B8));
        else if (o->sounds == 1) SDL_snprintf(s, sizeof s, "%s", O2S(m, 0x13C0));
        /* as on a Tandy or PCjr ([0541] set): the speaker, or the sound chip */
        else if (o->sounds == 2) SDL_snprintf(s, sizeof s, "%s", O2S(m, 0x10EB));   /* internal speaker */
        else                     SDL_snprintf(s, sizeof s, "%s", O2S(m, 0x10D9));   /* external speaker */
        break;
    case 4:                     /* WARET.EXE: an EGA or a VGA can't centre the screen */
        if (m->mode != MODE_CGA && gfx_card != CARD_TANDY)
            SDL_snprintf(s, sizeof s, "%s: no centering", gfx_card == CARD_EGA ? "ega" : "vga");
        else SDL_snprintf(s, sizeof s, "%s", O2S(m, 0x1363));
        break;
    default:
        if (o->joystick) SDL_snprintf(s, sizeof s, "%s%s", O2S(m, 0x13EE), O2S(m, 0xEB4));
        else             SDL_snprintf(s, sizeof s, "%s%s", O3S(m, 0xE5E), O2S(m, 0x13EE));
        break;
    }
    small_caps(m, 12, SHOPPE_Y[k], s);
}

static void page_shoppe(Menu *m)
{
    show_picture(m, PIC_OPTIONS_SHOPPE, false);
    for (int k = 0; k < 6; k++) shoppe_line(m, k);
    text_border(m, 2);                  /* ovl1:57F5: [A5C0] = 2, text_rows_add */
    for (int r = 23; r < 25; r++) clear_row(m, r);
    icon(m, true, I_PLUS, 0, 184);
    print_at(m, 3, 23, DSS(m, 0xCF));
    icon(m, true, I_MINUS, 12, 184);
    print_at(m, 9, 23, O2S(m, 0x407));                  /* to select an option to change. */
    icon(m, false, I_SPACEBAR, 0, 192);
    print_at(m, 7, 24, O2S(m, 0x426));                  /* to change, */
    icon(m, true, I_ENTER, 37, 192);
    print_at(m, 22, 24, O2S(m, 0x431));                 /* when done. */
}

/* battle_report (ovl1_53AE): PN or PO, and the numbers (ovl1_5706) */
static void report_number(Menu *m, int xb, int y, int v)
{
    if (v < 0 || v > 999) v = 999;
    char s[4];
    SDL_snprintf(s, sizeof s, "%3d", v);
    for (int i = 0; i < 3; i++, xb += 2) {
        glyph(m, m->cv.px, SCREEN_H, F_SPACE, xb * 4, y);
        if (s[i] != ' ') glyph(m, m->cv.px, SCREEN_H, F_DIGITS + (s[i] - '0') * 24, xb * 4, y);
    }
}

static void page_report(Menu *m)
{
    bool won = m->report_code < 10;
    show_picture(m, won ? PIC_REPORT_WIN : PIC_REPORT_LOSE, false);
    int f = 41 - ega_left(m);                /* WARET.EXE on an EGA or VGA: yours 4 pixels further left */
    report_number(m, f, 40, m->rep[0]);      /* 669h: captured, yours */
    report_number(m, 52, 40, m->rep[1]);     /* 674h */
    report_number(m, f, 52, m->rep[2]);      /* 849h: casualties */
    report_number(m, 52, 52, m->rep[3]);
    report_number(m, f, 66, m->rep[4]);      /* A79h: flags taken */
    report_number(m, 52, 66, m->rep[5]);
    if (won) {
        print_at(m, 6, 1, O2S(m, 0xF86));                         /* Victory was yours */
        int c = m->report_code;
        print_at(m, 6, 2, O2S(m, c == 0 ? 0xF98 : c == 1 ? 0xFAF : 0x10A8));
    } else {
        print_at(m, 6, 1, O2S(m, (unsigned)o2w(m, 0x10FD + 2 * m->rep_opp)));
        int c = m->report_code;
        print_at(m, 6, 2, O2S(m, c == 10 ? 0x107B : c == 11 ? 0x1093 : 0x10A8));
    }
    if (tune_once(m)) {
        if (won) snd_tune(m->ga->o2, 0x122F, 0x12C7, 0x122F, 0x1263, 0x1295, 0x12C7);   /* ovl1:55B2 */
        else     snd_tune(m->ga->o2, 0x12F9, 0x133D, 0x12F9, 0x1311, 0x1327, 0x133D);   /* ovl1:567A */
        m->step = 0;                   /* "Press any key" once the tune is over (menu_frame) */
    } else if (m->step == 1) {         /* redrawn after the tune: the prompt again */
        clear_row(m, 24);
        print_at(m, 6, 24, O2S(m, 0x10BE));                       /* Press any key to continue. */
    }
}

static void report_prompt(Menu *m)
{
    clear_row(m, 24);
    print_at(m, 6, 24, O2S(m, 0x10BE));                           /* Press any key to continue. */
    m->step = 1;
}

/* a black page with text (save_game) */
static void page_message(Menu *m)
{
    set_palette(m, m->mode == MODE_CGA ? CGA_PALETTE : CGA_PALETTE, 4, false);
    screen_fill(&m->cv, 0, 0, SCREEN_W, SCREEN_H, m->c4[0]);
    for (int r = 0; r < 2; r++) print_at(m, 0, 10 + r, m->message[r]);
}

/* the page for the current state, from scratch (after a mode change) */
static void redraw(Menu *m)
{
    m->npal = 0;
    switch (m->state) {
    case MS_PRESENTED: page_presented(m); break;
    case MS_TITLE: page_title(m); break;
    case MS_TITLES: case MS_ASK_SAVED:
        page_titles(m, false);
        if (m->state == MS_ASK_SAVED) {
            clear_row(m, 23); clear_row(m, 24);
            print_at(m, 0, 24, O3S(m, 0xE23));
        }
        break;
    case MS_STORY: case MS_RULES: {
        int keep_view = m->view, keep_rule = m->rule;
        if (m->state == MS_STORY) page_story(m); else page_rules(m);
        m->anim = 0;
        m->view = keep_view;
        m->rule = keep_rule;
        m->row = 3 + 2 * m->rule;
        scroll_show(m);
        break;
    }
    case MS_OPPONENT: page_opponent(m); break;
    case MS_QUOTE: page_opponent(m); page_quote(m); break;
    case MS_REPORT: page_report(m); break;
    case MS_CAMPAIGN_MENU: case MS_EDIT_MENU: page_campaign_menu(m); break;
    case MS_EDIT_TEXT: case MS_EDIT_ASK: page_edit_text(m, false); break;
    case MS_ERASE_ASK: case MS_ERASE:
        page_titles(m, false);
        clear_row(m, 23); clear_row(m, 24);
        if (m->state == MS_ERASE_ASK) print_at(m, 0, 24, O3S(m, 0xB80));
        else {
            for (int c = 0; c < m->ed_col; c++) glyph(m, m->sbuf, SCROLL_LINES, F_SPACE, TEXT_LEFT + c * 8, TEXT_TOP + m->row * ROW_STEP);
            scroll_copy(m, m->view, 0, VIEW_LINES);
        }
        break;
    case MS_NOTE:
        set_palette(m, CGA_PALETTE, 4, false);
        screen_fill(&m->cv, 0, 0, SCREEN_W, SCREEN_H, m->c4[0]);
        print_at(m, m->note_col, m->note_row, m->note);
        break;
    case MS_SHOPPE: page_shoppe(m); pointer(m, 3, SHOPPE_Y[m->shoppe_item]); break;
    default: page_message(m); break;
    }
    m->drawn = true;
}

/* ------------------------------------------------------------------ files */

static void data_path(const Menu *m, const char *name, char *out, size_t n)
{
    SDL_snprintf(out, n, "%s%s", m->data_dir, name);
}

static void root_path(const Menu *m, const char *name, char *out, size_t n)
{
    SDL_snprintf(out, n, "%s%s", m->data_root, name);
}

/* ------------------------------------------------------------------ data disks
 * The original kept campaigns and saved games on data disks, swapped in the
 * drive. Here each data disk is a folder in Data Disks/, and the one in the
 * drive is chosen in the Options Shoppe or when opening ( data disk ). A
 * floppy's files can be copied into a new folder as they are. */

static SDL_EnumerationResult SDLCALL disk_found(void *ud, const char *dir, const char *name)
{
    Menu *m = ud;
    char path[1200];
    SDL_snprintf(path, sizeof path, "%s%s/%s", m->data_root, DISKS_DIR, name);
    SDL_PathInfo info;
    if (name[0] != '.' && SDL_GetPathInfo(path, &info) && info.type == SDL_PATHTYPE_DIRECTORY && m->ndisks < MAX_DISKS)
        SDL_strlcpy(m->disks[m->ndisks++], name, sizeof m->disks[0]);
    (void)dir;
    return SDL_ENUM_CONTINUE;
}

static int disk_cmp(const void *a, const void *b) { return SDL_strcasecmp((const char *)a, (const char *)b); }

/* the files of a data disk: TITLES, S and T files named in TITLES (T0-TZ) */
static bool disk_file(const char *name)
{
    if (!SDL_strcasecmp(name, "TITLES") || !SDL_strcasecmp(name, "S") || !SDL_strcasecmp(name, "SET")) return true;
    return (name[0] == 'T' || name[0] == 't') && name[1] && SDL_isalnum((unsigned char)name[1]) && !name[2];
}

static SDL_EnumerationResult SDLCALL old_file_found(void *ud, const char *dir, const char *name)
{
    Menu *m = ud;
    if (disk_file(name)) {
        char from[1200], to[1200];
        SDL_snprintf(from, sizeof from, "%s%s", m->data_root, name);
        SDL_snprintf(to, sizeof to, "%s%s/Data Disk 1/%s", m->data_root, DISKS_DIR, name);
        SDL_RenamePath(from, to);
    }
    (void)dir;
    return SDL_ENUM_CONTINUE;
}

/* disk_error (ovl1_61fd): "Unable to use data disk." / "Press any key to
 * continue." on the bottom rows; everything waits for a key.  The remake
 * shows it when a file can't be written (the original's DOS disk errors);
 * menu_draw puts it over the screen, so the page under it is untouched. */
static void disk_error(Menu *m) { m->disk_error = true; }

static bool disk_write(Menu *m, const char *path, const void *d, size_t n)
{
    bool ok = SDL_SaveFile(path, d, n);
    if (!ok) disk_error(m);
    return ok;
}

static void disk_select(Menu *m, int i)
{
    if (m->ndisks == 0) return;
    m->disk = SDL_clamp(i, 0, m->ndisks - 1);
    SDL_snprintf(m->data_dir, sizeof m->data_dir, "%s%s/%s/", m->data_root, DISKS_DIR, m->disks[m->disk]);
    char path[1200];
    root_path(m, "DRIVE", path, sizeof path);           /* remembered for next time */
    disk_write(m, path, m->disks[m->disk], SDL_strlen(m->disks[m->disk]));
}

static void disks_scan(Menu *m)
{
    char dir[1200], path[1200];
    SDL_snprintf(dir, sizeof dir, "%s%s", m->data_root, DISKS_DIR);
    if (!SDL_GetPathInfo(dir, NULL)) {                  /* first time: the files kept before step 7 go on disk 1 */
        SDL_snprintf(path, sizeof path, "%s/Data Disk 1", dir);
        SDL_CreateDirectory(path);
        SDL_EnumerateDirectory(m->data_root, old_file_found, m);
    }
    m->ndisks = 0;
    SDL_EnumerateDirectory(dir, disk_found, m);
    if (m->ndisks == 0) {
        SDL_snprintf(path, sizeof path, "%s/Data Disk 1", dir);
        SDL_CreateDirectory(path);
        SDL_strlcpy(m->disks[m->ndisks++], "Data Disk 1", sizeof m->disks[0]);
    }
    SDL_qsort(m->disks, m->ndisks, sizeof m->disks[0], disk_cmp);
    int pick = 0;
    root_path(m, "DRIVE", path, sizeof path);
    char *name = SDL_LoadFile(path, NULL);
    for (int i = 0; name && i < m->ndisks; i++)
        if (!SDL_strcmp(m->disks[i], name)) pick = i;
    SDL_free(name);
    disk_select(m, pick);
}

/* a blank data disk: the original wrote TITLES (from DTITLES) on a formatted floppy */
static void disk_new(Menu *m)
{
    char name[64], path[1200];
    for (int n = 1; n < 100; n++) {
        SDL_snprintf(name, sizeof name, "Data Disk %d", n);
        SDL_snprintf(path, sizeof path, "%s%s/%s", m->data_root, DISKS_DIR, name);
        if (!SDL_GetPathInfo(path, NULL)) break;
    }
    if (!SDL_CreateDirectory(path)) { disk_error(m); return; }
    disks_scan(m);
    for (int i = 0; i < m->ndisks; i++)
        if (!SDL_strcmp(m->disks[i], name)) disk_select(m, i);
}

static void options_from_raw(MenuOptions *o)
{
    o->display = o->raw[1];
    o->sounds = o->raw[3] | (o->raw[4] << 8);
    o->fight_delay = o->raw[5] | (o->raw[6] << 8);
    o->joystick = o->raw[9] | (o->raw[10] << 8);
    o->data_drive = o->raw[23] | (o->raw[24] << 8);
    if (o->display > 2) o->display = 2;
    if (o->sounds < 0 || o->sounds > 3) o->sounds = 2;
    if (o->fight_delay != 200 && o->fight_delay != 400 && o->fight_delay != 800) o->fight_delay = 400;
}

static void options_to_raw(MenuOptions *o)
{
    o->raw[1] = (uint8_t)o->display;
    o->raw[3] = (uint8_t)o->sounds; o->raw[4] = (uint8_t)(o->sounds >> 8);
    o->raw[5] = (uint8_t)o->fight_delay; o->raw[6] = (uint8_t)(o->fight_delay >> 8);
    o->raw[9] = (uint8_t)o->joystick; o->raw[10] = (uint8_t)(o->joystick >> 8);
    o->raw[23] = (uint8_t)o->data_drive; o->raw[24] = (uint8_t)(o->data_drive >> 8);
}

/* ovl1_30F0: CNF; the remake keeps its own copy in the data folder (save_options) */
static void options_load(Menu *m)
{
    MenuOptions *o = &m->opt;
    SDL_memset(o, 0, sizeof *o);
    o->raw[1] = 2;                                  /* rgb */
    o->raw[3] = 2;                                  /* sounds on */
    o->raw[5] = 0x90; o->raw[6] = 0x01;             /* 400 */
    char path[1200];
    root_path(m, "CNF", path, sizeof path);
    size_t size = 0;
    uint8_t *d = SDL_LoadFile(path, &size);
    if (!d) d = assets_load_file(m->ga->dir, "CNF", &size);
    if (d && size >= 25) SDL_memcpy(o->raw, d, 25);
    SDL_free(d);
    options_from_raw(o);
}

static void options_save(Menu *m)
{
    options_to_raw(&m->opt);
    char path[1200];
    root_path(m, "CNF", path, sizeof path);
    disk_write(m, path, m->opt.raw, 25);
}

/* The data folder plays the data disk: TITLES there lists its campaigns
 * (made from DTITLES), and S holds the saved game. */
static bool data_titles_load(Menu *m)
{
    char path[1200];
    data_path(m, "TITLES", path, sizeof path);
    size_t size = 0;
    uint8_t *d = assets_load_file(m->data_dir, "TITLES", &size);   /* any case: copied from a floppy */
    if (!d || size < 876) {
        SDL_free(d);
        d = assets_load_file(m->ga->dir, "DTITLES", &size);   /* a new data disk */
        if (!d || size < 876) { SDL_free(d); return false; }
        disk_write(m, path, d, 876);
    }
    SDL_memcpy(m->data_titles, d, 876);
    SDL_free(d);
    return true;
}

#define SAVE_MAGIC "WARREMAKE-S1"

/* The saved game on the data disk: S (the remake's or WAR.EXE's) or SET (WARET.EXE's, the 16-colour
 * program's). A disk used by both programs can hold both: the one saved last is the game. */
static bool saved_file_info(const Menu *m, const char *name, SDL_PathInfo *info)   /* any case */
{
    int n = 0;
    char **found = SDL_GlobDirectory(m->data_dir, name, SDL_GLOB_CASEINSENSITIVE, &n);
    bool ok = false;
    if (found && n > 0) {
        char path[1200];
        data_path(m, found[0], path, sizeof path);
        ok = SDL_GetPathInfo(path, info) && info->type == SDL_PATHTYPE_FILE;
    }
    SDL_free(found);
    return ok;
}

static const char *saved_game_file(const Menu *m)
{
    SDL_PathInfo s, set;
    bool has_s = saved_file_info(m, "S", &s), has_set = saved_file_info(m, "SET", &set);
    if (has_s && has_set) return set.modify_time > s.modify_time ? "SET" : "S";
    return has_set ? "SET" : has_s ? "S" : NULL;
}

bool menu_has_saved_game(const Menu *m) { return saved_game_file(m) != NULL; }

static bool save_write(Menu *m, const Play *p)
{
    char path[1200];
    data_path(m, "S", path, sizeof path);
    size_t n = 12 + 8 + sizeof(Campaign) + sizeof(Play);
    uint8_t *buf = SDL_malloc(n);
    if (!buf) return false;
    SDL_memcpy(buf, SAVE_MAGIC, 12);
    uint32_t sc = (uint32_t)sizeof(Campaign), sp = (uint32_t)sizeof(Play);
    SDL_memcpy(buf + 12, &sc, 4);
    SDL_memcpy(buf + 16, &sp, 4);
    SDL_memcpy(buf + 20, p->c, sizeof(Campaign));
    SDL_memcpy(buf + 20 + sizeof(Campaign), p, sizeof(Play));
    bool ok = SDL_SaveFile(path, buf, n);
    SDL_free(buf);
    return ok;
}

/* A game saved by the original: its campaign's title and story, found by
 * its map among the WAR disk's campaigns and the data disk's */
static void original_campaign(Menu *m, Campaign *c)
{
    for (int i = 0; i < NUM_CAMPAIGNS; i++)
        if (m->camps[i].loaded && !SDL_memcmp(m->camps[i].map, c->map, sizeof c->map)) {
            SDL_strlcpy(c->title, m->camps[i].title, sizeof c->title);
            SDL_memcpy(c->story_raw, m->camps[i].story_raw, sizeof c->story_raw);
            campaign_refresh(c);
            return;
        }
    if (!data_titles_load(m)) return;
    for (int k = 1; k < 12; k++) {
        char name[4] = { (char)m->data_titles[k * 3], (char)m->data_titles[k * 3 + 1], 0, 0 };
        Campaign t;
        if (!campaign_load_file(&t, m->data_dir, name) || SDL_memcmp(t.map, c->map, sizeof c->map)) continue;
        int row = 3 + 2 * k, n = 0;
        char title[33];
        for (int col = 2; col < 31; col++) {
            uint8_t ch = m->data_titles[36 + row * 32 + col];
            if (ch >= 0x20 && ch < 0x7F) title[n++] = (char)ch;
        }
        while (n > 0 && title[n - 1] == ' ') n--;
        title[n] = 0;
        if (n) SDL_strlcpy(c->title, title, sizeof c->title);
        SDL_memcpy(c->story_raw, t.story_raw, sizeof c->story_raw);
        campaign_refresh(c);
        return;
    }
}

bool menu_load_saved_game(Menu *m, Play *p, Campaign *c)
{
    size_t size = 0;
    const char *file = saved_game_file(m);
    uint8_t *d = file ? assets_load_file(m->data_dir, file, &size) : NULL;
    if (d && (size == ORIGINAL_S_SIZE || size == ORIGINAL_SET_SIZE)) {   /* saved by WAR.EXE (ovl1:35C4) or WARET.EXE (ov1:066F) */
        bool imported = savegame_import(p, c, m->ga, d, size, (uint32_t)SDL_GetTicksNS());
        SDL_free(d);
        if (imported) original_campaign(m, c);
        return imported;
    }
    bool ok = d && size == 20 + sizeof(Campaign) + sizeof(Play) && SDL_memcmp(d, SAVE_MAGIC, 12) == 0;
    if (ok) {
        uint32_t sc, sp;
        SDL_memcpy(&sc, d + 12, 4);
        SDL_memcpy(&sp, d + 16, 4);
        ok = sc == sizeof(Campaign) && sp == sizeof(Play);
    }
    if (ok) {
        SDL_memcpy(c, d + 20, sizeof(Campaign));
        SDL_memcpy(p, d + 20 + sizeof(Campaign), sizeof(Play));
        p->c = c;
        p->ga = m->ga;
    }
    SDL_free(d);
    return ok;
}

/* The saved game on this disk can't be continued: say why, then the title page */
void menu_saved_game_failed(Menu *m)
{
    size_t size = 0;
    const char *file = saved_game_file(m);
    uint8_t *d = file ? assets_load_file(m->data_dir, file, &size) : NULL;
    SDL_free(d);
    SDL_memset(m->message, 0, sizeof m->message);
    if (size == ORIGINAL_S_SIZE || size == ORIGINAL_SET_SIZE) {   /* WAR.EXE's or WARET.EXE's data */
        SDL_strlcpy(m->message[0], "  This game was saved by the original,", sizeof m->message[0]);
        SDL_strlcpy(m->message[1], "  but the file could not be read.", sizeof m->message[1]);
    } else {
        SDL_strlcpy(m->message[0], "  This game was saved by another", sizeof m->message[0]);
        SDL_strlcpy(m->message[1], "  version of the remake.", sizeof m->message[1]);
    }
    go(m, MS_MESSAGE);
}

/* ------------------------------------------------------------------ the campaign editor (step 7)
 * Making a campaign takes six steps: the map, the squads and flags, the
 * enemy leader, the rules, the story and the title (rt_01A7, campaign_intro
 * ovl1_3E08). Changing one chooses it from the data disk, then any of six
 * scenes (rt_02B9). Its T file and TITLES are written to the data disk. */

static int count_titles(const uint8_t *text)   /* scroll_write_text [A8CD]: rows after ( war disk ) */
{
    int n = 0;
    for (int r = 2; r < 26; r++)
        if (text[r * 32 + 2] != ' ' || text[r * 32 + 3] != ' ') n++;
    return n - 1;
}

/* a data disk's file: the name it already has in any case, else this one */
static void disk_file_path(const Menu *m, const char *name, char *out, size_t n)
{
    int found = 0;
    char **list = SDL_GlobDirectory(m->data_dir, name, SDL_GLOB_CASEINSENSITIVE, &found);
    SDL_snprintf(out, n, "%s%s", m->data_dir, found > 0 ? list[0] : name);
    SDL_free(list);
}

static bool titles_save(Menu *m)               /* ovl1_7E09 */
{
    char path[1200];
    disk_file_path(m, "TITLES", path, sizeof path);
    return disk_write(m, path, m->data_titles, sizeof m->data_titles);
}

static bool edit_save(Menu *m)                 /* ovl1_7A71 (ovl1_3FC3 with 100) */
{
    campaign_refresh(&m->game);
    bool ok = campaign_save_file(&m->game, m->data_dir, m->edit_file);
    if (!ok) disk_error(m);                    /* the changes stay in m->game */
    return ok;
}

static void edit_reload(Menu *m)               /* ovl1_3FC3 with 101: forget the changes */
{
    campaign_load_file(&m->game, m->data_dir, m->edit_file);
}

/* ovl1_4FF2 and ovl1_4F55: a new campaign's rules and blank armies */
static void blank_campaign(Campaign *c, const char *file)
{
    SDL_memset(c, 0, sizeof *c);
    SDL_strlcpy(c->file, file, sizeof c->file);
    SDL_memset(c->story_raw, ' ', sizeof c->story_raw);            /* ovl1:3E23 */
    static const uint16_t RULES[RULE_COUNT] = { 1, 0, 2, 60, 2, 80, 20, 8, 6 };
    SDL_memcpy(c->rules, RULES, sizeof RULES);
    uint8_t *a = c->armies;
    for (int i = 0; i < MAX_SQUADS; i++) {
        a[80 + 2 * i] = 9999 & 0xFF; a[81 + 2 * i] = 9999 >> 8;    /* no squad */
        a[40 + 2 * i] = 2;                                         /* march */
        a[2 * i] = a[2 * i + 1] = 0xFF;                            /* friend */
        a[200 + i] = 1;                                            /* formation 1 */
    }
    for (int i = 0; i < MAX_FLAGS; i++) a[260 + 2 * i] = a[261 + 2 * i] = 0xFF;   /* no flag */
    campaign_refresh(c);
    c->loaded = true;
}

/* a line of text for a moment, then the title page */
static void note_set(Menu *m, int col, int row, const char *text)
{
    SDL_strlcpy(m->note, text, sizeof m->note);
    m->note_col = col;
    m->note_row = row;
}

static void note(Menu *m, int col, int row, const char *text, int frames)
{
    note_set(m, col, row, text);
    m->state = MS_NOTE;
    m->frame = 0;
    m->step = frames;
    print_at(m, col, row, text);
}

/* rt_01A7: the campaign menu's first three items */
static void edit_begin(Menu *m, int which)
{
    if (!data_titles_load(m)) { note(m, 0, 24, "Unable to use data disk.", 2 * MENU_HZ); return; }
    int count = count_titles(m->data_titles + 36);
    m->edit = which;
    if (which == EDIT_NEW) {                       /* data_disk (ovl1_4EB2) */
        int index = count + 1;
        if (index > 11) {
            snd_error();
            clear_row(m, 24);
            note(m, 0, 24, O3S(m, 0x9EB), (int)(30000 * SOUND_UNIT_US * 1e-6 * MENU_HZ));   /* Can't add any more campaigns! */
            m->edit = EDIT_NONE;
            return;
        }
        m->sel = index;                            /* [A694] */
        char name[4] = { (char)m->data_titles[index * 3], (char)m->data_titles[index * 3 + 1], 0, 0 };
        SDL_strlcpy(m->edit_file, name, sizeof m->edit_file);
        blank_campaign(&m->game, name);
        m->opp = 0;                                /* [A67C] */
        mapedit_start(&m->mapedit, m->ga, &m->game, true);
        go(m, MS_MAPEDIT);
        return;
    }
    if (count < 1) {                               /* the remake: nothing to change or remove */
        clear_row(m, 24);
        note(m, 0, 24, "No campaigns on this data disk.", 2 * MENU_HZ);
        m->edit = EDIT_NONE;
        return;
    }
    m->sel = 1;                                    /* choose_title: the first campaign, not ( war disk ) */
    go(m, MS_TITLES);
    m->step = 1;
}

static void edit_menu(Menu *m)                 /* editor_menu (ovl1_3EB6) */
{
    m->a692 = 0;
    go(m, MS_EDIT_MENU);
}

/* ------------------------------------------------------------------ scroll_editor (ovl1_6A78) */

static int ed_pos(const Menu *m) { return m->ed_row * 32 + m->ed_col; }

static void ed_row_up(Menu *m)                 /* scroll_row_up (ovl1_6EDE) */
{
    if (--m->ed_row < 0) { m->ed_row = 0; return; }
    int base = (m->a692 == 0 || m->a692 == 10) ? 52 : 88;
    if (m->view + base > TEXT_TOP + m->ed_row * ROW_STEP && m->view >= ROW_STEP) m->view -= ROW_STEP;
}

static void ed_row_down(Menu *m)               /* scroll_row_down (ovl1_6F6D) */
{
    if (++m->ed_row > 25) { m->ed_row = 25; return; }
    if (TEXT_TOP + m->ed_row * ROW_STEP >= m->view + 142 && m->view + ROW_STEP <= SCROLL_LINES - VIEW_LINES) m->view += ROW_STEP;
}

static void ed_back(Menu *m)                   /* ovl1_7059 */
{
    if (--m->ed_col < 0) {
        if (m->ed_row <= 0) m->ed_col = 0;
        else { m->ed_col = 30; ed_row_up(m); }
    }
    if (m->text[ed_pos(m)] == 1) ed_back(m);
}

static void ed_advance(Menu *m)                /* scroll_advance (ovl1_6FE0) */
{
    if (++m->ed_col > 30) {
        if (m->ed_row >= 25) m->ed_col--;
        else { m->ed_col = 0; ed_row_down(m); }
    } else if (m->text[ed_pos(m)] == 1) {
        ed_advance(m);                             /* past the second cell of a capital */
    }
}

/* ovl1_70C7: a letter in the cell; a capital takes the next cell as well */
static void ed_put(Menu *m, int c)
{
    uint8_t *t = m->text;
    int p = ed_pos(m);
    if (is_upper(t[p]) && p + 1 < 840) t[p + 1] = ' ';
    if (is_upper(c)) {
        if (p + 1 < 840) t[p + 1] = 1;
        if (p + 2 < 840 && t[p + 2] == 1) t[p + 2] = ' ';
    }
    t[p] = (uint8_t)c;
}

static void ed_show(Menu *m)
{
    scroll_text(m);
    scroll_show(m);
}

/* the rows under the scroll while writing */
static void ed_help(Menu *m)
{
    for (int r = 23; r < 25; r++) clear_row(m, r);
    if (m->a692 == 3) {                            /* ovl1_4DD9 */
        if (m->ed_anim) return;
        print_at(m, 0, 23, O3S(m, 0x9C2));         /* The last step is to write a title. */
        print_at(m, 0, 24, O3S(m, 0x9E5));         /* Press [Enter] when done. */
        icon(m, true, I_ENTER, 0x0C, 192);
        print_at(m, 9, 24, O3S(m, 0x939));
    } else if (m->ed_wait) {                       /* ovl1_4D52 */
        print_at(m, 0, 23, O3S(m, 0x99C));         /* The fifth step is to write a story. */
        char line[48];
        SDL_snprintf(line, sizeof line, "%s%s", O3S(m, 0x9E5), O3S(m, 0xEC6));
        print_at(m, 0, 24, line);                  /* Press any key. */
    } else {                                       /* ovl1_4D9E: [Esc] when done. */
        icon(m, false, I_ESC, 0, 192);
        print_at(m, 4, 24, O3S(m, 0x939));
    }
}

/* show_scroll with [A692] 0 (a new story), 10 (change the story) or 3 (a new title) */
static void page_edit_text(Menu *m, bool unroll)
{
    if (unroll) {
        if (m->a692 == 3) SDL_memcpy(m->text, m->data_titles + 36, 840);
        else SDL_memcpy(m->text, m->game.story_raw, 840);
        m->view = 0;
        m->ed_typed = 0;
        if (m->a692 == 3) {                        /* scroll_goto(2, 3), then down past the titles */
            m->ed_col = 2; m->ed_row = 3;
            m->ed_anim = m->sel;
        } else {
            m->ed_col = 0; m->ed_row = 0;
            m->ed_anim = 0;
        }
        m->ed_wait = m->a692 == 0;
    }
    scroll_palette(m);
    ed_help(m);
    if (unroll) scroll_open(m);
    else {
        scroll_palette(m);
        scroll_build(m);
        scroll_show(m);
        if (m->state == MS_EDIT_ASK) {
            clear_row(m, 23); clear_row(m, 24);
            print_at(m, 0, 24, O3S(m, 0xDAE));
        }
    }
}

/* ovl1_740F: the keys the scroll has no letter for */
static bool ed_allowed(int c)
{
    return c >= 0x20 && c < 0x7F && !SDL_strchr("\\><~@#$%^&*=+`", c);
}

static void ed_done(Menu *m)
{
    if (m->a692 == 0) {                            /* a new story: write the T file, then the title */
        SDL_memcpy(m->game.story_raw, m->text, 840);
        edit_save(m);
        m->a692 = 3;
        scroll_close(m, MS_EDIT_TEXT);
    } else if (m->a692 == 10) {                    /* ovl1:3A70: Save the last changes? */
        clear_row(m, 23); clear_row(m, 24);
        print_at(m, 0, 24, O3S(m, 0xDAE));
        m->state = MS_EDIT_ASK;
        m->step = 0;
        m->frame = 0;
    } else {                                       /* a title: TITLES, and the campaign is complete */
        SDL_memcpy(m->data_titles + 36, m->text, 840);
        titles_save(m);
        m->edit = EDIT_NONE;
        note_set(m, 6, 12, O3S(m, 0xC9A));         /* ovl1:3E98: THIS CAMPAIGN IS COMPLETE */
        scroll_close(m, MS_NOTE);
    }
}

static void ed_type(Menu *m, int c)            /* ovl1:6C70 */
{
    if (m->a692 == 3 && m->ed_col == 2 && c == ' ') return;   /* no space before a title */
    if (m->text[ed_pos(m)] == 1) m->ed_col--;
    if (c > ' ') m->ed_typed++;
    else if (m->text[ed_pos(m)] > ' ' && m->ed_typed) m->ed_typed--;
    ed_put(m, c);
    if (!(m->a692 == 3 && m->ed_col >= 29)) ed_advance(m);
    ed_show(m);
}

static void ed_key(Menu *m, int k)
{
    if (m->anim || m->ed_anim) return;
    if (m->ed_wait) {                              /* any key */
        snd_beep();
        m->ed_wait = false;
        ed_help(m);
        scroll_show(m);
        return;
    }
    bool title = m->a692 == 3;
    switch (k) {
    case PK_ESC:                                   /* when done (not a title) */
        if (title) return;
        snd_beep();
        ed_done(m);
        return;
    case PK_ENTER:
        if (title) { if (m->ed_typed) { snd_beep(); ed_done(m); } return; }
        if (m->ed_row < 25) { m->ed_col = 0; ed_row_down(m); }
        break;
    case PK_BACKSPACE:
        if (m->ed_col == 0 && m->ed_row == 0) return;
        if (title) {
            if (m->ed_col >= 29) ed_put(m, ' ');
            if (m->ed_col <= 2) { ed_show(m); return; }
        }
        ed_back(m);
        if (m->ed_typed && m->text[ed_pos(m)] > ' ') m->ed_typed--;
        ed_put(m, ' ');
        break;
    case PK_LEFT:
        if (!title || m->ed_col > 2) ed_back(m);
        break;
    case PK_RIGHT:
        if (!title || (m->ed_col < 29 && (m->ed_col > 2 || m->text[ed_pos(m)] != ' '))) ed_advance(m);
        break;
    case PK_UP:   if (!title) ed_row_up(m); break;
    case PK_DOWN: if (!title) ed_row_down(m); break;
    default:
        return;
    }
    ed_show(m);
}

/* the title has been chosen to be removed: wipe it off (ovl1_4125) */
static void erase_step(Menu *m)
{
    for (int k = 0; k < 2 && m->ed_col < 33; k++, m->ed_col++) {
        int x = TEXT_LEFT + m->ed_col * 8, y = TEXT_TOP + m->row * ROW_STEP;
        glyph(m, m->sbuf, SCROLL_LINES, F_SPACE, x, y);
        snd_tone(20000, 20);                       /* speaker_click */
    }
    scroll_copy(m, m->view, 0, VIEW_LINES);
    if (m->ed_col < 33) return;
    /* select_campaign (ovl1_4000): its file name goes to the end of the
     * table, the titles below move up a row pair */
    int idx = m->sel, count = count_titles(m->data_titles + 36);
    uint8_t *tab = m->data_titles;
    if (idx < 11) {
        uint8_t name[3];
        SDL_memcpy(name, tab + idx * 3, 3);
        SDL_memmove(tab + idx * 3, tab + (idx + 1) * 3, (size_t)(11 - idx) * 3);
        SDL_memcpy(tab + 11 * 3, name, 3);
    }
    uint8_t *t = m->data_titles + 36;
    int pos = m->row * 32;
    if (pos < 767) SDL_memmove(t + pos, t + pos + 64, (size_t)(767 - pos));
    int clear = count * 64 + 0x5F;
    if (clear + 32 <= 840) SDL_memset(t + clear, ' ', 32);
    titles_save(m);
    m->edit = EDIT_NONE;
    scroll_close(m, MS_TITLE);
}

/* F: the formations taught in the remake are kept in its own folder and
 * used in place of the game's F (the original wrote F to the WAR disk) */
static void formations_load(Menu *m)
{
    char path[1100];
    root_path(m, "F", path, sizeof path);
    size_t n = 0;
    uint8_t *f = SDL_LoadFile(path, &n);
    if (f && n == sizeof m->ga->formations) SDL_memcpy(m->ga->formations, f, n);
    SDL_free(f);
}

static void formations_save(Menu *m, uint8_t f[420])   /* ovl5_454E */
{
    formed_sort(f);
    char path[1100];
    root_path(m, "F", path, sizeof path);
    disk_write(m, path, f, 420);
    SDL_memcpy(m->ga->formations, f, 420);         /* used this session even if F wasn't written */
}

void menu_held(Menu *m, int held) { m->held = held; }
bool menu_wants_held(const Menu *m)
{
    if (m->disk_error) return false;
    return m->state == MS_SQEDIT || (m->state == MS_FORMED && formed_wants_held(&m->formed));
}
bool menu_wants_text(const Menu *m)
{
    if (m->disk_error) return false;
    return m->state == MS_EDIT_TEXT && m->drawn && !m->anim && !m->ed_anim && !m->ed_wait;
}

void menu_text(Menu *m, const char *utf8)
{
    if (!menu_wants_text(m)) return;
    for (const unsigned char *c = (const unsigned char *)utf8; *c; c++)
        if (ed_allowed(*c)) ed_type(m, *c);
}

/* ------------------------------------------------------------------ public */

Menu *menu_create(GameAssets *ga, Campaign *camps, const char *data_dir)
{
    Menu *m = SDL_calloc(1, sizeof *m);
    if (!m) return NULL;
    m->ga = ga;
    m->camps = camps;
    SDL_strlcpy(m->data_root, data_dir ? data_dir : "", sizeof m->data_root);
    formations_load(m);
    m->rnd = (uint32_t)SDL_GetTicksNS();
    options_load(m);
    disks_scan(m);
    sound_setting(m->opt.sounds);      /* A6E1 */
    return m;
}

void menu_destroy(Menu *m) { SDL_free(m); }

void menu_start(Menu *m) { go(m, MS_PRESENTED); }
void menu_title(Menu *m) { m->title_item = 0; m->edit = EDIT_NONE; go(m, MS_TITLE); }

void menu_report(Menu *m, const Play *p)
{
    m->report_code = p->result_code;
    m->rep[0] = p->captured_mine;  m->rep[1] = p->captured_theirs;    /* A6F9, A6FB */
    m->rep[2] = p->lost_mine;      m->rep[3] = p->lost_theirs;        /* A6FD, A6FF */
    m->rep[4] = p->flags_taken;    m->rep[5] = p->flags_captured;     /* A703, A701 */
    m->rep_opp = p->c ? p->c->opponent : 0;
    go(m, MS_REPORT);
}

/* save_game (ovl1_35C4) */
void menu_save(Menu *m, const Play *p)
{
    m->save_from = p;
    SDL_memset(m->message, 0, sizeof m->message);
    if (menu_has_saved_game(m)) {
        SDL_strlcpy(m->message[0], O2S(m, 0xF07), sizeof m->message[0]);   /* WARNING: Any previously saved game on */
        SDL_strlcpy(m->message[1], O2S(m, 0xF30), sizeof m->message[1]);   /* your data disk will be erased. */
        go(m, MS_SAVE_WARN);
    } else {
        m->step = 0;
        go(m, MS_SAVED);
    }
}

MenuAction menu_action(Menu *m) { MenuAction a = m->action; m->action = MENU_NONE; return a; }
const Campaign *menu_game(const Menu *m) { return &m->game; }
const MenuOptions *menu_options(const Menu *m) { return &m->opt; }
void menu_set_display(Menu *m, int display) { m->opt.display = SDL_clamp(display, 0, 2); }   /* the card's change redraws the page */
uint32_t menu_seed(const Menu *m) { return m->seed; }

static bool key_down(int k) { return k == PK_EQUALS || k == PK_KP_PLUS || k == PK_CTRL || k == PK_DOWN || k == PK_RIGHT; }
static bool key_up(int k)   { return k == PK_MINUS || k == PK_KP_MINUS || k == PK_TAB || k == PK_UP || k == PK_LEFT; }

/* ( data disk ): read the disk's TITLES (ovl1_7B3D); a saved game asks first */
static void open_data_disk(Menu *m)
{
    if (!data_titles_load(m)) return;
    m->data_list = true;
    m->sel = 0;
    if (menu_has_saved_game(m)) {             /* Saved game / New campaigns */
        page_titles(m, false);
        clear_row(m, 23); clear_row(m, 24);
        print_at(m, 0, 24, O3S(m, 0xE23));
        snd_beep_high();                      /* ovl1:7C0D */
        m->state = MS_ASK_SAVED;
    } else page_titles(m, false);
}

static void choose_campaign(Menu *m)
{
    /* select_campaign / ovl1_7870: the T file of the chosen title */
    const uint8_t *src = m->data_list ? m->data_titles : m->ga->titles;
    char name[4] = { (char)src[m->sel * 3], (char)src[m->sel * 3 + 1], 0, 0 };
    int idx = -1;
    for (int c = 0; c < NUM_CAMPAIGNS; c++)
        if (m->camps[c].loaded && SDL_strcasecmp(m->camps[c].file, name) == 0) idx = c;
    if (m->data_list) {                   /* a campaign on the data disk: its T file is in the data folder */
        if (!campaign_load_file(&m->game, m->data_dir, name)) return;
        char title[33];                   /* its title, from the disk's TITLES */
        int n = 0;
        for (int c = 2; c < 31; c++) {
            uint8_t ch = m->data_titles[36 + (3 + 2 * m->sel) * 32 + c];
            if (ch >= 0x20 && ch < 0x7F) title[n++] = (char)ch;
        }
        while (n > 0 && title[n - 1] == ' ') n--;
        title[n] = 0;
        if (n) SDL_strlcpy(m->game.title, title, sizeof m->game.title);
    } else {
        if (idx < 0) return;
        m->game = m->camps[idx];
    }
    m->opp = m->game.opponent;
    scroll_close(m, MS_STORY);
}

void menu_key(Menu *m, int k)
{
    if (!k) return;
    if (m->disk_error) { m->disk_error = false; return; }   /* ovl1:622A: any key */
    switch (m->state) {
    case MS_PRESENTED:
        sound_stop();                      /* the remake: a key skips the tune too */
        go(m, MS_TITLE);
        break;
    case MS_TITLE:
        if (key_down(k)) { snd_beep(); m->title_item = (m->title_item + 1) % 3; title_box(m); }   /* ovl1:43A9 */
        else if (key_up(k)) { snd_beep(); m->title_item = (m->title_item + 2) % 3; title_box(m); }
        else if (k == PK_ENTER) {
            snd_beep();                                        /* ovl1:43EA */
            if (m->title_item == 0) { m->sel = 0; m->data_list = false; go(m, MS_TITLES); m->step = 1; }
            else if (m->title_item == 1) { m->menu_item = 0; go(m, MS_CAMPAIGN_MENU); }
            else m->action = MENU_QUIT;
        }
        break;
    case MS_ASK_SAVED:
        if (k == PK_S) {                                       /* Saved game */
            snd_beep();                                        /* ovl1:7C27 */
            m->action = MENU_RESUME_GAME;
        } else if (k == PK_N) {                                /* New campaigns */
            snd_beep();
            go(m, MS_TITLES);
        }
        break;
    case MS_TITLES:
        if (m->anim) break;
        if (k == PK_ESC && m->disk_list) { m->disk_list = false; m->sel = 11; page_titles(m, false); break; }
        if (k == PK_ESC) {
            if (m->edit) { snd_beep(); m->edit = EDIT_NONE; }   /* ovl1:652D: changed your mind */
            scroll_close(m, MS_TITLE);
            break;
        }
        if (key_up(k) && m->sel > (m->a692 == 5 || m->a692 == 7 ? 1 : 0)) {
            snd_beep();                                        /* ovl1:64E1 */
            m->sel--;
            m->row -= 2;
            while (TEXT_TOP + m->row * ROW_STEP - m->view < 88 && m->view > 0) m->view -= ROW_STEP;
            scroll_show(m);
        } else if (key_down(k) && m->sel < m->count) {
            snd_beep();                                        /* ovl1:6515 */
            m->sel++;
            m->row += 2;
            while (TEXT_TOP + m->row * ROW_STEP - m->view > 142) m->view += ROW_STEP;
            scroll_show(m);
        } else if (k == PK_ENTER) {
            snd_beep();                                        /* ovl1:63E9 */
            if (m->a692 == 7) {                               /* the campaign to change (rt_02B9) */
                char name[4] = { (char)m->data_titles[m->sel * 3], (char)m->data_titles[m->sel * 3 + 1], 0, 0 };
                SDL_strlcpy(m->edit_file, name, sizeof m->edit_file);
                if (!campaign_load_file(&m->game, m->data_dir, name)) {
                    clear_row(m, 23); clear_row(m, 24);
                    note(m, 0, 24, "Unable to use data disk.", 2 * MENU_HZ);
                    m->edit = EDIT_NONE;
                    break;
                }
                m->a692 = 0;
                scroll_close(m, MS_EDIT_MENU);
            } else if (m->a692 == 5) {                        /* Erase this title? (Y or N) */
                clear_row(m, 23); clear_row(m, 24);
                print_at(m, 0, 24, O3S(m, 0xB80));
                m->state = MS_ERASE_ASK;
            } else if (m->disk_list) {                               /* the remake: this disk goes in the drive */
                disk_select(m, m->sel);
                m->disk_list = false;
                open_data_disk(m);
            } else if (!m->data_list && m->sel == 11) {       /* ( data disk ) */
                if (m->ndisks > 1) {                          /* which one? */
                    m->disk_list = true;
                    m->sel = m->disk;
                    page_titles(m, false);
                } else open_data_disk(m);
            } else if (m->data_list && m->sel == 0) {         /* ( war disk ) */
                m->data_list = false;
                m->sel = 0;
                page_titles(m, false);
            } else {
                choose_campaign(m);
            }
        }
        break;
    case MS_STORY:
        if (m->anim) break;
        if (k == PK_ESC) { m->state = MS_TITLES; m->step = 1; m->drawn = false; break; }
        if (k == PK_ENTER) { snd_beep(); scroll_close(m, MS_RULES); break; }   /* ovl1:6280 */
        if (key_up(k) && m->view > 0) { m->view -= 4; scroll_show(m); }
        if (key_down(k) && m->view < 216) { m->view += 4; scroll_show(m); }
        break;
    case MS_RULES:
        if (m->anim) break;
        if (m->ed_wait) {                                      /* a new campaign's fourth step: any key */
            snd_beep();
            m->ed_wait = false;
            rules_help(m);
            scroll_show(m);
            break;
        }
        if (k == PK_ESC) { if (!m->edit) { go(m, MS_STORY); m->step = 1; } break; }
        if (k == PK_ENTER) {
            snd_beep();                                        /* ovl1:683D */
            if (m->edit == EDIT_NEW) { m->a692 = 0; scroll_close(m, MS_EDIT_TEXT); }   /* the story next */
            else if (m->edit == EDIT_CHANGE) { edit_save(m); scroll_close(m, MS_EDIT_MENU); }   /* ovl1:3C04 */
            else { scroll_close(m, MS_OPPONENT); m->opp_phase = 0; }
            break;
        }
        if (key_up(k) && m->rule > 0) {
            snd_beep();                                        /* ovl1:6888 */
            m->rule--; m->row -= 2;
            while (TEXT_TOP + m->row * ROW_STEP - m->view < 88 && m->view > 0) m->view -= ROW_STEP;
            scroll_show(m);
        } else if (key_down(k) && m->rule < 8) {
            snd_beep();                                        /* ovl1:68B8 */
            m->rule++; m->row += 2;
            while (TEXT_TOP + m->row * ROW_STEP - m->view > 142) m->view += ROW_STEP;
            scroll_show(m);
        } else if (k == PK_SPACE) {
            snd_beep();                                        /* ovl1:68D4 */
            rule_change(m, m->rule);
            rules_text(m);
            scroll_text(m);
            scroll_show(m);
        }
        break;
    case MS_OPPONENT:
        if (m->sweep >= 0) break;
        if (m->frame_on) opponent_frame(m);
        if (m->opp_phase == 2) {                               /* the third step: any key (ovl1:49B3) */
            snd_beep();
            m->opp_phase = 1;
            clear_row(m, 24);
            help_change_select(m, O3S(m, 0x991), 40, 23, O3S(m, 0xBF6));
            opponent_text(m);
            m->frame = 0;
            break;
        }
        if (k == PK_ESC) { if (!m->edit) { go(m, MS_RULES); m->step = 1; } break; }
        if (k == PK_ENTER && m->edit) {
            snd_beep();                                        /* ovl1:4CD4 */
            m->game.opponent = m->opp;
            if (m->edit == EDIT_NEW) { m->ed_wait = true; go(m, MS_RULES); m->step = 1; }   /* the rules next */
            else { edit_save(m); edit_menu(m); }               /* ovl1:4CF2 */
            break;
        }
        if (k == PK_ENTER) {
            snd_beep();                                        /* ovl1:4CD4 */
            m->game.opponent = m->opp;                        /* memcpy(A931[OPPONENT], A4DC, 38) */
            m->seed = (uint32_t)SDL_GetTicksNS();
            go(m, MS_QUOTE);
            m->drawn = true;
            page_quote(m);
            break;
        }
        if (m->opp_phase == 0) {
            if (key_down(k)) {
                snd_beep();                                    /* ovl1:4B11 */
                m->opp = (m->opp + 1) & 7;
                m->opp_phase = 1;
                help_change_select(m, O3S(m, 0x991), 40, 23, O3S(m, 0xBF6));
                clear_row(m, 0); clear_row(m, 1);
                m->sweep = 0; m->step = 0;
                xor_box(m, 0, 0, 18, 6);
            }
        } else if (key_down(k) || key_up(k)) {
            snd_beep();                                        /* ovl1:4B59 */
            m->opp = key_down(k) ? (m->opp + 1) & 7 : (m->opp + 7) & 7;
            clear_row(m, 0); clear_row(m, 1);
            m->sweep = 0; m->step = 0;
            xor_box(m, 0, 0, 18, 6);
        }
        m->frame = 0;
        break;
    case MS_QUOTE:
        m->action = MENU_START_GAME;
        break;
    case MS_DEMO:                          /* a key while the demo loads ends it at once */
        menu_title(m);
        break;
    case MS_MESSAGE:
        menu_title(m);
        break;
    case MS_REPORT:
        if (m->step < 1) break;                                /* the tune is still playing */
        snd_beep();                                            /* ovl1:56FC */
        menu_title(m);
        break;
    case MS_CAMPAIGN_MENU: case MS_EDIT_MENU: {
        if (m->sweep >= 0) break;
        if (m->frame_on) menu_frame_box(m);
        int *item = scene_item(m);
        if (key_down(k) || key_up(k)) {
            snd_beep();                                        /* ovl1:6088 */
            *item = key_down(k) ? (*item + 1) % 6 : (*item + 5) % 6;
            clear_row(m, 0); clear_row(m, 1);
            m->sweep = 0; m->step = 0;
            xor_box(m, 0, 0, 18, 6);
        } else if (k == PK_ENTER && m->state == MS_EDIT_MENU) {
            snd_beep();
            switch (m->edit_item) {                            /* rt_02B9 */
            case 0: go(m, MS_RULES); m->step = 1; break;      /* RULEBOOK */
            case 1: m->a692 = 10; go(m, MS_EDIT_TEXT); break; /* STORYBOOK */
            case 2: mapedit_start(&m->mapedit, m->ga, &m->game, false); go(m, MS_MAPEDIT); break;   /* MAP MAKER */
            case 3: menu_title(m); break;                     /* EXIT */
            case 4: m->opp = m->game.opponent & 7; m->opp_phase = 1; go(m, MS_OPPONENT); break;    /* ENEMY */
            default: sqedit_start(&m->sqedit, m->ga, &m->game, false); go(m, MS_SQEDIT); break;     /* ARMY */
            }
            break;
        } else if (k == PK_ENTER) {
            switch (m->menu_item) {
            case 0: snd_beep(); edit_begin(m, EDIT_NEW); break;      /* Make a new campaign */
            case 1: snd_beep(); edit_begin(m, EDIT_CHANGE); break;   /* Change a campaign */
            case 2: snd_beep(); edit_begin(m, EDIT_REMOVE); break;   /* Remove a campaign */
            case 3: menu_title(m); break;                     /* Return to title page */
            case 5: m->shoppe_item = m->mode != MODE_CGA; m->opt_changed = false; m->shoppe_disk = m->disk; go(m, MS_SHOPPE); break;
            default:                                           /* Teach squads new formations (thunk 12) */
                snd_beep();
                formed_start(&m->formed, m->ga, m->ga->formations);
                go(m, MS_FORMED);
                break;
            }
            break;
        }
        m->frame = 0;
        break;
    }
    case MS_EDIT_TEXT:
        ed_key(m, k);
        break;
    case MS_EDIT_ASK:                                          /* ovl1:3A8C: Y or N */
        if (m->anim || m->step) break;
        if (k == PK_Y) { print_at(m, 32, 24, O3S(m, 0xE5A)); m->ask_yes = true; m->step = 1; m->frame = 0; }
        else if (k == PK_N) { print_at(m, 32, 24, DSS(m, 0x9C)); m->ask_yes = false; m->step = 1; m->frame = 0; }
        break;
    case MS_ERASE_ASK:                                         /* ovl1:645D: Y or N */
        if (k == PK_N) { snd_beep(); m->state = MS_TITLES; page_titles(m, false); }
        else if (k == PK_Y) { clear_row(m, 23); clear_row(m, 24); m->ed_col = 0; m->state = MS_ERASE; }
        break;
    case MS_MAPEDIT:
        mapedit_key(&m->mapedit, k);
        break;
    case MS_SQEDIT:
        sqedit_key(&m->sqedit, k);
        break;
    case MS_FORMED:
        formed_key(&m->formed, k);
        break;
    case MS_NOTE:
        menu_title(m);
        break;
    case MS_SHOPPE:
        if (k == PK_ENTER) {
            snd_beep();                                        /* options_shoppe ovl1:58D6-5A31 */
            if (m->opt_changed) options_save(m);
            if (m->shoppe_disk >= m->ndisks) disk_new(m);
            else if (m->shoppe_disk != m->disk) disk_select(m, m->shoppe_disk);
            menu_title(m);
            break;
        }
        if (key_up(k) && m->shoppe_item > (m->mode != MODE_CGA)) {   /* WARET.EXE: no display line to change */
            snd_beep();
            m->shoppe_item--;
            if (m->shoppe_item == 4) m->shoppe_item--;        /* "center the screen" does nothing here */
            page_shoppe(m);
            pointer(m, 3, SHOPPE_Y[m->shoppe_item]);
        } else if (key_down(k) && m->shoppe_item < 5) {
            snd_beep();
            m->shoppe_item++;
            if (m->shoppe_item == 4) m->shoppe_item++;
            page_shoppe(m);
            pointer(m, 3, SHOPPE_Y[m->shoppe_item]);
        } else if (k == PK_SPACE) {
            MenuOptions *o = &m->opt;
            snd_beep();
            switch (m->shoppe_item) {
            case 0: if (m->mode == MODE_CGA) o->display = o->display >= 2 ? 0 : o->display + 1; break;
            case 1: o->fight_delay = o->fight_delay >= 800 ? 200 : o->fight_delay << 1; break;
            case 2: m->shoppe_disk = (m->shoppe_disk + 1) % (m->ndisks + 1); break;   /* the next disk, or a new one */
            case 3: o->sounds = o->sounds >= 3 ? 0 : o->sounds + 1; sound_setting(o->sounds); break;
            case 5: o->joystick = !o->joystick; break;
            default: break;
            }
            m->opt_changed = true;
            page_shoppe(m);
            pointer(m, 3, SHOPPE_Y[m->shoppe_item]);
        }
        break;
    case MS_SAVE_WARN:                                         /* Continue   Quit */
        if (k == PK_C) { m->step = 0; go(m, MS_SAVED); }
        else if (k == PK_Q) m->action = MENU_CONTINUE;
        break;
    case MS_SAVED:                                             /* <C>ontinue with game or <Q>uit? */
        if (m->step < 2) break;
        if (k == PK_C) m->action = MENU_CONTINUE;
        else if (k == PK_Q) menu_title(m);
        break;
    default:
        break;
    }
}

void menu_frame(Menu *m)
{
    if (m->disk_error) return;             /* WAR.EXE waits in ovl1_61fd */
    if (!m->drawn) return;                 /* menu_draw makes the page first */
    m->frame++;
    switch (m->state) {
    case MS_PRESENTED:                     /* ovl1_startup: until the tune ends, or 60000 delay units */
        if (m->frame > (int)(60000 * SOUND_UNIT_US * 1e-6 * MENU_HZ) && sound_busy() <= 0.0) go(m, MS_TITLE);
        break;
    case MS_TITLE:                         /* the box blinks (ovl1_4206): measured in DOSBox, */
        if (m->frame % 28 == 15) { xor_box(m, 17, 54, 25, 5); m->box_inverted = true; }    /* 0.75 s plain, */
        if (m->frame % 28 == 0 && m->box_inverted) {                                       /* 0.65 s inverted */
            xor_box(m, 17, 54, 25, 5);
            m->box_inverted = false;
        }
        if (++m->idle >= DEMO_IDLE_FRAMES) {
            if (m->box_inverted) { xor_box(m, 17, 54, 25, 5); m->box_inverted = false; }
            demo_start(m);
        }
        break;
    case MS_DEMO:
        if (m->frame == DEMO_MOMENT) { clear_row(m, 24); print_at(m, 10, 24, DSS(m, 0x26)); }   /* rt:06C1 */
        if (m->frame >= DEMO_LOAD_FRAMES) { m->action = MENU_DEMO; m->state = MS_TITLE; m->drawn = false; }
        break;
    case MS_TITLES: case MS_STORY: case MS_RULES: case MS_ERASE_ASK:
        if (m->anim && scroll_anim(m)) go(m, m->next_state);
        break;
    case MS_EDIT_TEXT:
        if (m->anim) { if (scroll_anim(m)) go(m, m->next_state); break; }
        if (m->ed_anim) {                  /* ovl1_4DD9: the quill goes down past the titles */
            ed_row_down(m);
            ed_row_down(m);
            if (--m->ed_anim == 0) ed_help(m);
            scroll_show(m);
        }
        break;
    case MS_EDIT_ASK:                      /* "Yes" or "No" shows a moment, then the scroll rolls up */
        if (m->anim) { if (scroll_anim(m)) go(m, m->next_state); break; }
        if (m->step == 1 && m->frame > 8) {
            if (m->ask_yes) { SDL_memcpy(m->game.story_raw, m->text, 840); edit_save(m); }
            else edit_reload(m);
            m->step = 2;
            scroll_close(m, MS_EDIT_MENU);
        }
        break;
    case MS_ERASE:
        if (m->anim) { if (scroll_anim(m)) go(m, m->next_state); break; }
        erase_step(m);
        break;
    case MS_MAPEDIT: {
        MapEdit *e = &m->mapedit;
        mapedit_frame(e);
        if (e->result == MAPEDIT_RUNNING) break;
        if (e->result == MAPEDIT_ABORT) { menu_title(m); break; }
        if (e->result != MAPEDIT_DISCARD) SDL_memcpy(m->game.map, e->map, sizeof m->game.map);
        if (e->result == MAPEDIT_NEXT) {   /* the second step */
            sqedit_start(&m->sqedit, m->ga, &m->game, true);
            go(m, MS_SQEDIT);
        } else {
            if (e->result == MAPEDIT_SAVE) edit_save(m); else edit_reload(m);
            edit_menu(m);
        }
        break;
    }
    case MS_SQEDIT: {
        SquadEdit *e = &m->sqedit;
        sqedit_frame(e, m->held);
        if (e->result == SQEDIT_RUNNING) break;
        if (e->result != SQEDIT_DISCARD) sqedit_store(e, &m->game);
        if (e->result == SQEDIT_NEXT) {    /* the third step: the enemy leader */
            m->opp = 0;
            m->opp_phase = 2;
            go(m, MS_OPPONENT);
        } else {
            if (e->result == SQEDIT_SAVE) edit_save(m); else edit_reload(m);
            edit_menu(m);
        }
        break;
    }
    case MS_NOTE:
        if (m->frame > m->step) menu_title(m);
        break;
    case MS_FORMED: {
        FormEdit *e = &m->formed;
        formed_frame(e, m->held);
        if (e->save) { formations_save(m, e->f); e->save = false; }
        if (e->done) menu_title(m);
        break;
    }
    case MS_OPPONENT: case MS_CAMPAIGN_MENU: case MS_EDIT_MENU: {
        bool opp = m->state == MS_OPPONENT;
        if (opp && m->opp_phase == 2) break;
        if (m->message_back && --m->message_back == 0) page_campaign_menu(m);
        if (m->sweep >= 0) {               /* the box sweeps across the top rows: */
            xor_box(m, m->sweep, 0, 18, 6);    /* about 0.15 s in all, measured in DOSBox */
            m->sweep += 20;
            if (m->sweep < 58) {
                xor_box(m, m->sweep, 0, 18, 6);
            } else {
                m->sweep = -1;
                if (opp) opponent_text(m);
                else scene_text(m);
                m->frame = 0;
            }
            break;
        }
        int t = m->frame % 4;              /* the frame flickers: 0.1 s off, 0.1 s on (measured in DOSBox) */
        if (t == 2 && !m->frame_on) { if (opp) opponent_frame(m); else menu_frame_box(m); }
        if (t == 0 && m->frame_on) { if (opp) opponent_frame(m); else menu_frame_box(m); }
        break;
    }
    case MS_QUOTE:
        if (m->frame > 4 * MENU_HZ) m->action = MENU_START_GAME;
        break;
    case MS_REPORT:
        if (m->step == 0 && sound_busy() <= 0.0) report_prompt(m);
        break;
    case MS_MESSAGE:
        if (m->frame > 4 * MENU_HZ) menu_title(m);
        break;
    case MS_SAVED:
        if (m->step == 0) {
            SDL_memset(m->message, 0, sizeof m->message);
            SDL_strlcpy(m->message[0], "             Saving Game", sizeof m->message[0]);
            page_message(m);
            m->step = 1;
        } else if (m->step == 1 && m->frame > MENU_HZ / 2) {
            bool ok = m->save_from && save_write(m, m->save_from);
            SDL_memset(m->message, 0, sizeof m->message);
            SDL_snprintf(m->message[0], sizeof m->message[0], "   %s", DSS(m, ok ? 0x74 : 0x64));
            if (!ok) SDL_snprintf(m->message[1], sizeof m->message[1], "   %s", DSS(m, 0x74));
            page_message(m);
            m->step = 2;
        }
        break;
    default:
        break;
    }
}

/* ovl1_61fd's two rows, over whatever the screen shows */
static void draw_disk_error(const Menu *m, Screen *s, GfxMode mode)
{
    uint8_t fg = 3, bg = 0;
    if (mode != MODE_CGA) {
        const RGB *cga = m->pal0 ? CGA_PALETTE_0 : CGA_PALETTE;
        fg = 15;                                    /* WARET.EXE's text rows */
        bg = nearest(s->pal, 16, cga[0]);
    }
    screen_fill(s, 0, 23 * 8, SCREEN_W, 16, bg);
    screen_text(s, m->ga, 0, 23 * 8, DSS(m, 0xD6), fg, bg);       /* Unable to use data disk. */
    screen_text(s, m->ga, 0, 24 * 8, DSS(m, 0xEF), fg, bg);       /* Press any key to continue. */
}

static void menu_draw_page(Menu *m, Screen *s, GfxMode mode);

void menu_draw(Menu *m, Screen *s, GfxMode mode)
{
    menu_draw_page(m, s, mode);
    if (m->disk_error) draw_disk_error(m, s, mode);
}

static void menu_draw_page(Menu *m, Screen *s, GfxMode mode)
{
    if (m->state == MS_MAPEDIT || m->state == MS_SQEDIT || m->state == MS_FORMED) {   /* the editors draw the whole screen */
        m->mode = mode;
        m->card = gfx_card;
        m->drawn = true;
        if (m->state == MS_MAPEDIT) mapedit_draw(&m->mapedit, s, mode);
        else if (m->state == MS_SQEDIT) sqedit_draw(&m->sqedit, s, mode);
        else formed_draw(&m->formed, s, mode);
        return;
    }
    if (!m->drawn && m->state == MS_NOTE && (mode == m->mode && gfx_card == m->card)) {   /* on what the screen shows */
        clear_row(m, 23); clear_row(m, 24);                        /* ovl1:3CFC */
        print_at(m, m->note_col, m->note_row, m->note);
        m->drawn = true;
        m->step = (int)(2.5 * MENU_HZ);
    }
    if (!m->drawn && m->state == MS_EDIT_TEXT && (mode == m->mode && gfx_card == m->card)) {
        page_edit_text(m, true);
        m->drawn = true;
    }
    if ((mode != m->mode || gfx_card != m->card) || !m->drawn) {
        bool first = !m->drawn;
        /* Tab to a 16-colour card with the pointer on the display line, which WARET.EXE's pointer can't reach */
        if (m->state == MS_SHOPPE && mode != MODE_CGA && m->shoppe_item == 0) m->shoppe_item = 1;
        m->mode = mode;
        m->card = gfx_card;
        if (first && (m->state == MS_TITLES || m->state == MS_STORY || m->state == MS_RULES) && m->step == 1) {
            /* a scroll page opening from the last page */
            if (m->state == MS_TITLES) page_titles(m, true);
            else if (m->state == MS_STORY) page_story(m);
            else page_rules(m);
            m->step = 0;
            m->drawn = true;
        } else if (first && m->state == MS_STORY) {
            page_story(m);
            m->drawn = true;
        } else if (first && m->state == MS_RULES) {
            page_rules(m);
            m->drawn = true;
        } else if (first && m->state == MS_OPPONENT) {
            page_opponent(m);
            m->drawn = true;
        } else if (first && (m->state == MS_SAVE_WARN)) {
            page_message(m);
            clear_row(m, 24);
            print_at(m, 0, 24, O2S(m, 0xF59));                  /* Continue   Quit */
            m->drawn = true;
        } else {
            redraw(m);
            if (m->state == MS_SAVE_WARN) { clear_row(m, 24); print_at(m, 0, 24, O2S(m, 0xF59)); }
        }
    }
    SDL_memcpy(s->px, m->cv.px, sizeof s->px);
    for (int i = 0; i < 16; i++) s->pal[i] = m->cv.pal[i];
}
