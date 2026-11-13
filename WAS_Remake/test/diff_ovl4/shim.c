/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/diff_ovl4/shim.c: the remake's campaign play as a library for the
 * Python differential tests (diff.py, calib.py)
 */
#include "assets.h"
#include "campaign.h"
#include "play_int.h"
#include "dsmap.h"
#include <string.h>

static GameAssets ga;
static Campaign cs[NUM_CAMPAIGNS];
static Play p;

int  shim_init(const char *dir) { char e[256]; if (!assets_load(&ga, dir, e, sizeof e)) return 0; campaigns_load(cs, dir); return 1; }
void shim_start(int c, unsigned seed) { play_start(&p, &cs[c], &ga, seed); }
void shim_frames(int n, int held) { for (int i = 0; i < n; i++) { if (p.stall) { p.stall--; continue; } play_frame(&p, held); } }
void shim_key(int k) { play_key(&p, k, false); }
void shim_dump(uint8_t *ds) { play_to_ds(&p, ds); }
void shim_load(const uint8_t *ds) { play_from_ds(&p, ds); }
int  shim_mode(void) { return p.mode; }
int  shim_result(void) { return p.result; }
void shim_set_time(int div) { p.time_div = div; }
void shim_enemy_turn(void) { enemy_turn(&p); }
void shim_strength_map(void) { enemy_strength_map(&p); }
void shim_port_values(void) { enemy_port_values(&p); }
int  shim_route(int sq, int target) { p.ai_sq = sq; p.ai_target = target; return enemy_route(&p); }
void shim_join(int sq) { p.ai_sq = sq; enemy_join(&p); }
int  shim_call(int which, int arg) { return play_debug_call(&p, which, arg); }
void shim_ev(uint32_t *out) { for (int i = 0; i < EV_COUNT; i++) out[i] = play_ev[i]; }
void shim_ev_reset(void) { for (int i = 0; i < EV_COUNT; i++) play_ev[i] = 0; }
int  shim_ev_count(void) { return EV_COUNT; }
