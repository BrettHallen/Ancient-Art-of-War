/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * play_int.h: the routines of campaign play (play.c) that the enemy
 * (enemy.c) shares
 */
#ifndef WAS_PLAY_INT_H
#define WAS_PLAY_INT_H

#include "play.h"

#define OPP(p, addr) ((p)->opp[((addr) - 0x604D) / 2])   /* the opponent's value at DS:addr */

/* What WAS.EXE's work in a tick costs on an XT: events counted by the
 * routines (EV), weighed in 8086 instructions by work_instructions() */
enum { EV_RND, EV_LIST, EV_SMALL, EV_HELP, EV_PORT, EV_S40, EV_C200, EV_QUEUE, EV_NEIGH, EV_STEP, EV_RING, EV_PRING, EV_TILE, EV_BASE,
       EV_SEAQ,     /* sea_way (ovl4:E5BF): a cell's moved along the queue */
       EV_SEAPOP,   /* sea_way: a cell taken from the queue */
       EV_SEAB,     /* sea_way: the way back, a cell moved along the queue */
       EV_COUNT };
extern uint32_t play_ev[EV_COUNT];
#define EV(k, n) (play_ev[k] += (uint32_t)(n))

int  rnd(Play *p);                                   /* random() rt:2F0D */
void delay(Play *p, int units);                      /* delay() rt:2F6E */
bool commandable(const Play *p, int i);              /* rt:0B3D */
void list_ships(Play *p, int i);                     /* rt:0630 */
int  next_slot(int i);                               /* path_next_slot rt:19BC */
int  map_at(const Play *p, int cell);
bool tile_land(int t);                               /* rt:14A6 */
bool tile_rough(int t);                              /* rt:15C4 */
bool tile_shallow(int t);                            /* rt:1656 */
bool tile_sea(int t);                                /* rt:12BD */
int  dir_off(const Play *p, int d);                  /* DS:0784: the cell offset of direction d */
bool off_map(int cell, int col, int d);              /* rt:1C52 */
int  coast_block(const Play *p, int cell, int d);    /* rt:1C0C */
int  coast_pass(const Play *p, int d, int cell);     /* rt:1EF4 */
bool way_blocked(const Play *p, int d, int cell, int col);   /* rt:1E10 */
int  repair_port(const Play *p, int cell, bool most);        /* rt:1AA8 */
int  supply_port(const Play *p, int cell, bool most);        /* rt:19D6 */
int  crown_beside(const Play *p, int cell, int side);        /* rt:1B7D */
void invert(Play *p, int cell);                      /* ovl4:C35C */
int  cell_mid_x(const Play *p, int cell);            /* ovl4:AEAE */
int  cell_mid_y(const Play *p, int cell);            /* ovl4:AF08 */
int  heading_of(int step);                           /* step_heading ovl4:9A3C */
void clear_paths(Play *p, int i);                    /* rt:0F55 */
int  count_at(const Play *p, int cell, int side, bool any);  /* count_at_cell ovl4:D549 */
bool close_to(Play *p, int a, int b);                /* ovl4:C5B9 */
int  worst_damage(Play *p, int i);                   /* ovl4:EE4E */
void port_call(Play *p, int cell, int side);         /* ovl4:BD86 */
int  join_partner(Play *p, int a);                   /* ovl4:B252 */
int  find_free_run(const Play *p);                   /* ovl4:ECD1 */
void redraw_work(const Play *p);                     /* the timing of rt:104A + rt:1084 */
void port_sources(Play *p);                          /* ovl4:C07C: the ports merchants sail to */

/* enemy.c */
void enemy_turn(Play *p);                            /* ovl4:C79A */
int  play_debug_call(Play *p, int which, int arg);   /* the tests: single routines (play.c) */
void enemy_strength_map(Play *p);                    /* ovl4:E805 */
void enemy_port_values(Play *p);                     /* ovl4:EA51 */
void enemy_join(Play *p);                            /* ovl4:ED83 */
int  enemy_route(Play *p);                           /* ovl4:DB21 */
int  enemy_port_side(const Play *p, int cell);       /* ovl4:BD3C */

#endif
