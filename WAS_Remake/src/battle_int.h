/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * battle_int.h: what the battle's source files share (not for the rest of
 * the program). battle.c: setting up, the main loop, the keys and the end;
 * battle_ships.c: sailing, the guns, collisions and the ships' orders;
 * battle_gfx.c: WAS.EXE's drawing routines on the CGA screen.
 */
#ifndef WAS_BATTLE_INT_H
#define WAS_BATTLE_INT_H

#include "battle.h"

#define SO6   0x4AF5        /* SO6 is loaded here (the battle's texts and small pictures) */
#define SO7   0x4257        /* SO7 (the gun crew's pictures) */
#define PANEL 0x3F3D        /* the gun crew's panel buffer: 44 bytes x 18 lines */

/* the data segment copy */
static inline int  mb(const Battle *b, int a)  { return b->m[a & 0xFFFF]; }                   /* unsigned byte */
static inline int  msb(const Battle *b, int a) { return (int8_t)b->m[a & 0xFFFF]; }           /* signed byte (cbw) */
static inline int  mw(const Battle *b, int a)  { return (int16_t)(b->m[a & 0xFFFF] | (b->m[(a + 1) & 0xFFFF] << 8)); }
static inline void setmb(Battle *b, int a, int v) { b->m[a & 0xFFFF] = (uint8_t)v; }

/* battle.c */
void b_delay(Battle *b, int units);       /* delay() rt:2F6E: 50 us a unit on an XT */
void b_cost(Battle *b, double us);        /* time WAS.EXE spends working */
int  b_getkey(Battle *b);                 /* rt_2e8e: the next character, 0 if none */
int  b_random(Battle *b);                 /* random() rt:2F0D, 0-127 */
void b_beep(Battle *b);                   /* beep_low and the speaker (step 8) */
void b_sound(Battle *b, int divisor);     /* rt_2f3f: the speaker's note */
void b_sound_off(Battle *b);              /* rt_2f67 */
void b_click(Battle *b);                  /* speaker_click */
int  b_scnf_byte(const Battle *b, int ds);
int  b_scnf_word(const Battle *b, int ds);
int  ovl5_input(Battle *b);               /* ovl5_7ac6 */
void ovl6_message(Battle *b, int a, int c, int d, int sound);   /* ovl6_f898 (a, c, d: DS offsets or 0) */
void ovl6_panel(Battle *b);               /* ovl6_ddc7 */
void ovl6_commands(Battle *b);            /* ovl6_e15d */
void ovl6_change_prompt(Battle *b);       /* ovl6_e394 */
void ovl6_fire_label(Battle *b);          /* ovl6_e266 */
void ovl6_order_text(Battle *b);          /* ovl6_e3e3 */
void ovl6_speed_icon(Battle *b);          /* ovl6_df4b */
void ovl6_compass(Battle *b);             /* ovl6_dfe2 */
void ovl6_show_ship(Battle *b);           /* ovl6_e651 */
void ovl6_time_text(Battle *b);           /* ovl6_e31e */
void ovl6_menu_mark(Battle *b);           /* ovl6_fc39 */
void ovl6_sprites(Battle *b);             /* ovl6_f9ce */
void ovl6_background(Battle *b);          /* ovl6_fb95 */
void ovl6_crew_anim(Battle *b);           /* ovl6_e7f1 */
void ovl6_crew_panel(Battle *b);          /* ovl6_ed60 */
void ovl6_anim_tick(Battle *b);           /* ovl6_ef34 */
void ovl6_error_beep(Battle *b);          /* ovl6_f9b1 */
int  ovl6_plus_minus(Battle *b, int k);   /* ovl6_fcaa */
int  rt_next_ship(Battle *b);             /* rt_5783 */
void rt_flag_tune(Battle *b);             /* rt_5d3d */
bool rt_battle_over(Battle *b);           /* rt_51cf */
void rt_view_from_sel(Battle *b);         /* rt_550d */
void rt_blink_sel(Battle *b);             /* rt_559c */
int  ovl7_boarding(Battle *b, int o, int s); /* ovl7_c9b0: boarding; returns the ship that won */

/* battle_ships.c */
void ovl5_draw_ships(Battle *b);          /* ovl5_afc6 */
void ovl5_sail(Battle *b, int check);     /* ovl5_b4c5 */
int  ovl5_fire(Battle *b, int s);         /* ovl5_936d */
int  ovl6_xoff(Battle *b, int s);         /* ovl6_f939: the picture's x offset for the heading */
void ovl6_speeds(Battle *b);              /* ovl6_f0d2 */
void ovl6_particles(Battle *b, int s);    /* ovl6_f4b5 */
void ovl6_mimic(Battle *b, int s);        /* ovl6_f6f9 */
int  ovl6_eff_speed(Battle *b, int s);    /* ovl6_f0a1 */
int  rt_distance(Battle *b, int a, int c);/* rt_4926 */
int  rt_nearest_target(Battle *b, int s, int kind);   /* rt_433e */
bool rt_struck_left(Battle *b);           /* rt_44b2 */
bool rt_can_aim(Battle *b, int s);        /* rt_49ab */
int  rt_aim(Battle *b, int s, int side);  /* rt_3fa4 */
void rt_ai_step(Battle *b);               /* rt_34bf */
int  rt_enemy_fire(Battle *b);            /* rt_3eaa */
int  rt_skill(Battle *b, int k, int s);   /* rt_5199 */
int  rt_closest_own(Battle *b);           /* rt_58bc */
bool rt_enemy_near(Battle *b);            /* rt_597e */
void rt_sort_ships(Battle *b);            /* rt_5290 */
void rt_draw_all(Battle *b);              /* rt_5367 */
int  rt_fit_ships(Battle *b);             /* rt_585c */
void ovl6_turn_followers(Battle *b, int s);   /* ovl6_f80b */

/* battle_gfx.c */
int  row_addr(int row);
void g_clear_row(Battle *b, int row);     /* clear_text_row */
void g_goto(Battle *b, int col, int row); /* goto_xy */
void g_putc(Battle *b, int c);            /* rt_32d4 */
void g_print(Battle *b, int ds);          /* print_string (a DS offset) */
void g_prints(Battle *b, const char *s);
void g_number(Battle *b, int n);          /* print_number */
void g_spaces(Battle *b, int n);          /* print_spaces */
void g_hline(Battle *b, int line, int x, int len);   /* draw_hline */
void g_vline(Battle *b, int line, int x, int n);     /* draw_vline */
void g_icon(Battle *b, int src, int line, int x);    /* draw_icon */
void g_invert_block(Battle *b, int addr, int w, int h);   /* invert_block */
void g_palette(Battle *b, int n);         /* set_cga_palette_if */
void g_panel_copy(Battle *b, int off);    /* rt_6697 */
void g_panel_sprite(Battle *b, int spr, int off, int half, int over);   /* rt_656f */
void g_mask(Battle *b, int spr, int y, int colour, int x, int half);    /* rt_617a */
void g_erase(Battle *b, int spr, int y, int x, int hull);               /* rt_6242 */
void g_box(Battle *b, int x, int y, int on);                            /* rt_62dc */
void g_sea(Battle *b);                    /* rt_5dc5 */
void g_shore_overview(Battle *b, int rle);/* rt_5de5 */
void g_shore_close(Battle *b, int rle, int xoff);   /* rt_5e16 */
void g_ship(Battle *b, int spr, int y, int set, int x, int half, int sails, int type, int hull);   /* rt_5ed1 */
void g_mast(Battle *b, int set, int dmg); /* rt_6344 */
void g_speed_bar(Battle *b, int n);       /* rt_64f0 */
void g_needle(Battle *b, int spr, int x, int row);   /* rt_660c */
void g_fill(Battle *b, int x, int line, int w, int h, int v);   /* rt_66e2 (AAh), rt_6712 (FFh) */
void g_bar3f(Battle *b, int x, int line, int n);     /* rt_673e */
void g_build_shore(Battle *b);            /* ovl5_8d89 */

#endif
