/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * menu_int.h: overlay 1's machine (menu.c) and the routines the editors
 * share with it (edmap.c the map maker, edships.c the ships editor).
 * The routines keep WAS.EXE's short names; as campaign play has routines
 * of the same names (play_int.h), they are macros for mn_ functions here.
 */
#ifndef WAS_MENU_INT_H
#define WAS_MENU_INT_H

#include "menu.h"
#include "fiber.h"

/* A campaign as the editors keep it, in WAS.EXE's data segment (load_campaign
 * ovl1:C388 reads an S file into it, write_campaign_file ovl1:C527 writes it) */
typedef struct {
    uint8_t  map[200];                         /* 5BFF */
    int16_t  side[40], speed[40], x[40], y[40];   /* 5CDD (-1 friend, 0 foe), 5D2D (9999 none), 5D7D, 5DCD */
    int8_t   supplies[40], ship_sq[40], sails[40], type[40];   /* 5E1D, 5E45 (-1 none), 5E6D, 5E95 */
    uint8_t  names[40][15];                    /* 609F */
    int8_t   hull[40];                         /* 6025 */
    uint8_t  mark[40];                         /* 5EBD: 1 a flagship's squadron */
} Fleet;

#define S_SIZE 2180                            /* the S file write_campaign_file writes */

#define BUF_SIZE  0x7D00          /* BUF: field 0 at 0, field 1 at 3E80h */
#define FIELD     0x3E80
#define D19       290             /* [0D19]: WAS.EXE's calibrated delay count (the title page waits 7.3 s for the demo) */
#define SS_SIZE   0x28CC          /* the saved game: DS:3F39-6804 */
#define SS_START  0x3F39
#define MAX_DISKS 12
#define DISKS_DIR "Data Disks"

/* WAS.EXE's own codes */
#define R_BACK    0x270B          /* Esc: the page before */
#define R_AGAIN   0x270A          /* the scroll again (the other disk) */
#define R_FAIL    0x270F

/* SO3: the scrolls' letters (loaded at DS:3F3D, [648B]) */
#define F_UPPER   0x000           /* 'A'-'Z': 16 x 10, 44 bytes each */
#define F_LOWER   0x478           /* 'a'-'z': 8 x 8, 20 bytes each (small capitals) */
#define F_QUEST   0x680
#define F_EXCL    0x694
#define F_SPACE   0x6A8           /* 8 x 10 of parchment: clears a cell */
#define F_COMMA   0x6C0
#define F_QUOTES  0x6CC
#define F_PERIOD  0x6D8
#define F_DIGITS  0x6E0           /* '0'-'9': 24 bytes each */
#define F_POINTER 0x8B4           /* the sword, AND-drawn */
#define F_COLON   0xCF2
#define F_RPAREN  0xD06
#define F_LPAREN  0xD1A
#define F_DASH    0xD2E
#define F_SLASH   0xD42
#define F_UNDER   0xD56
#define F_BAR     0xD6E
#define I_ESC     0xCBE           /* key icons in SO3 */
#define I_SPACE   0xC36
#define I_ENTER   0x62B           /* key icons in WAS.EXE's data segment */
#define I_PLUS    0x64D
#define I_MINUS   0x66F

/* What overlay 1's ported routines are asked to do (WAS.EXE's main loop, rt:0000) */
enum { E_START, E_TITLE, E_REPORT, E_SAVE };

struct Menu {
    GameAssets *ga;
    Campaign   *camps;
    char        data_root[1024];  /* the data folder: SCNF, DRIVE and Data Disks/ */
    char        data_dir[1100];   /* the data disk "in the drive": Data Disks/<name>/ */
    char        disks[MAX_DISKS][64];
    int         ndisks, disk;
    int         shoppe_disk;      /* the Options Shoppe's choice (ndisks: a new one) */

    Fiber       fiber;
    bool        fiber_on;
    int         entry;
    double      clock_us, target_us;
    MenuAction  action;
    bool        on_title;

    /* the machine */
    uint8_t     vram[80 * SCREEN_H];          /* the CGA screen, a line after another */
    uint8_t     v16[SCREEN_W * SCREEN_H];     /* the same in 16 colours */
    uint8_t     buf[BUF_SIZE];                /* BUF */
    uint8_t     b16[SCREEN_W * 400];          /* BUF in 16 colours, a line after another */
    uint8_t     c16[2][4];                    /* CGA colours (palette 1, palette 0) in the 16 */
    int         view;                         /* ([3F39] - BUF) / 80: the line pair at the top */
    bool        pal0;                         /* CGA palette 0 (the scrolls) */
    int         rows;                         /* [645B]: text rows below the picture */
    int         blit_lines;                   /* 100 + [6459] / 2 */
    int         ccol, crow;                   /* the text position [0831], [0833] */
    uint16_t    rnd, rnd_ptr;                 /* random(): DS:0808, 080A */
    int         beeps;

    /* the keyboard: [0804] the last key pressed (taken by menu_poll_key), [0802] the key down */
    int         q804[1], n804;
    int         k802;
    bool        up802;
    bool        shift;                        /* [081A]: Shift held */

    /* overlay 1's variables */
    int         a6485;                        /* which page the scroll is */
    int         a6487;                        /* the title chosen */
    int         a6497;                        /* the disk in the drive: 2 the WAS disk, 1 a data disk */
    int         a64DD;                        /* titles on the scroll */
    int         a64D1, a64D3, a64D5, a64D7;   /* the scroll's writing position (BUF offsets) */
    int         a6870;                        /* the text cell (an index into story) */
    int         a6469;
    int         a6471;                        /* the campaign menu's item */
    bool        a6467;                        /* the scroll's pictures are in BUF */
    uint8_t     story[0x348 + 64];            /* DS:4E53: the scroll's text, 26 rows of 32 */
    uint8_t     names[36];                    /* DS:0BB5: the disk's S file names */
    bool        disk_list;                    /* the remake: the scroll lists the data disks */
    Campaign    game;
    int         opponent;                     /* [646F] */
    bool        demo;

    /* the battle report */
    int         rep_code, rep[6], rep_opp;

    /* saving and loading */
    uint8_t     ss[SS_SIZE];
    bool        have_ss;
    const Play *save_from;

    /* step 7, the editors */
    Fleet       ed;                           /* the campaign being made or changed, as in WAS.EXE's DS */
    int         a6473;                        /* the change menu's item */
    int         a64D9;                        /* the map maker's page */
    int         a64DB, a64CF, a64CB, a64CD;   /* the ships editor: on a squadron, the Info item, the cursor's blink */
    int         nships;                       /* [64D3]: the ships editor's count of ships */
    int         a65AA;                        /* the ships editor's state: 0, 1 on a squadron, 20 moving it */
    int         sq;                           /* [62FB]: the squadron looked at */
    int8_t      list[3];                      /* [6456]: its ships (list_ships) */
    int         counts[6];                    /* [64BF-64C9]: line, frigate, flag ships of friend and foe */
    int         mk_x, mk_y, mk_addr;          /* [5CC7], [5CC9], [5CCB]: the marker (bytes, line pairs, BUF offset) */
    bool        mk_on, sq_on;                 /* the marker and the squadrons are drawn in BUF (composed at blit) */
    bool        text_on;                      /* rt_313c: the BIOS keyboard (letters typed) */
    int         dosq[32], ndos;               /* its buffer, as DOS reads it (rt_2e8e) */
    bool        complete;                     /* "THIS CAMPAIGN IS COMPLETE" shown */
};

#define WORK(m, n) ((m)->clock_us += (n) * (1000.0 / 300.0))   /* n 8086 instructions at 300 a millisecond */

#define maybe_yield     mn_maybe_yield
#define delay           mn_delay
#define beep_low        mn_beep_low
#define poll_key        mn_poll_key
#define key802          mn_key802
#define keys_clear      mn_keys_clear
#define key_down        mn_key_down
#define key_up          mn_key_up
#define scr_byte        mn_scr_byte
#define scr_xor         mn_scr_xor
#define buf_byte        mn_buf_byte
#define blit            mn_blit
#define blit_menu       mn_blit_menu
#define gotoxy          mn_gotoxy
#define clear_row       mn_clear_row
#define clear_all_rows  mn_clear_all_rows
#define put_char        mn_put_char
#define print           mn_print
#define print_at        mn_print_at
#define icon_ds         mn_icon_ds
#define draw_icon_from  mn_draw_icon_from
#define text_rows_add   mn_text_rows_add
#define show_one_moment mn_show_one_moment
#define plus_or_minus   mn_plus_or_minus
#define palette         mn_palette
#define hline           mn_hline
#define DS              mn_DS
#define text_begin      mn_text_begin
#define text_end        mn_text_end
#define dos_key         mn_dos_key

void maybe_yield(Menu *m);
void delay(Menu *m, int units);                 /* rt:2F6E */
void beep_low(Menu *m);                         /* rt:0C32 */
int  poll_key(Menu *m);                         /* [0804], once (menu_poll_key ovl1:8AA9) */
int  key802(Menu *m);                           /* [0802]: the key held down */
void keys_clear(Menu *m);
bool key_down(int k);                           /* is_scroll_down_key rt_126A */
bool key_up(int k);                             /* is_scroll_up_key rt_1217 */
void scr_byte(Menu *m, int line, int xb, uint8_t b);
void scr_xor(Menu *m, int line, int xb, uint8_t mask);
void buf_byte(Menu *m, int a, uint8_t b);
void blit(Menu *m, int lines);                  /* rt_2715 */
void blit_menu(Menu *m);
void gotoxy(Menu *m, int col, int row);         /* rt:2EBF */
void clear_row(Menu *m, int row);               /* rt:2C48 */
void clear_all_rows(Menu *m);
void put_char(Menu *m, int c);                  /* rt_32d4 */
void print(Menu *m, const char *s);             /* rt:3355 */
void print_at(Menu *m, int col, int row, const char *s);
void icon_ds(Menu *m, unsigned off, int y, int xb);
void draw_icon_from(Menu *m, const uint8_t *src, size_t size, unsigned off, int y, int xb);   /* rt_2cc4 */
void text_rows_add(Menu *m, int n);             /* rt:0BC2 */
void show_one_moment(Menu *m);                  /* ovl1:8A2F */
void plus_or_minus(Menu *m);                    /* ovl1_bec5 */
void palette(Menu *m, bool pal0);
void hline(Menu *m, int line, int xb, int len); /* rt:2B95 */
const char *DS(const Menu *m, unsigned off);
void mspk_on(Menu *m);                          /* the speaker at overlay 1's clock (sound.c) */
void mspk_freq(Menu *m, int divisor);
void mspk_off(Menu *m);
void mspk_click(Menu *m);
void mtone(Menu *m, int divisor, int units);    /* speaker_on, rt_2f3f, delay, rt_2f67 */
void text_begin(Menu *m);                       /* rt_313c: the BIOS's keyboard, letters typed */
void text_end(Menu *m);                         /* rt_3110: WAS.EXE's own again */
int  dos_key(Menu *m);                          /* rt_2e8e: a typed character, 27 then the scan code for a cursor key, or 0 */

/* edmap.c, edships.c */
int  map_maker(Menu *m);                        /* ovl3_7650 */
int  ships_editor(Menu *m);                     /* ovl2_7650 */
void ed_list_ships(Menu *m, int sq);            /* list_ships rt:0630 */
void ed_overlay(Menu *m, int lines);            /* the squadrons and the marker, as BUF is shown */

#endif
