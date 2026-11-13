/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * enemy.c: the enemy commander on the map (WAS.EXE overlay 4, ovl4:C79A-
 * EF2E), ported routine by routine: four times a tick the next anchored
 * enemy squadron (or merchant) is given orders: stay to resupply or repair
 * in port, guard its crown, attack the nearest squadron it can beat, or
 * pick a target (your crown, a flagship, a port, a weak spot on the strength
 * map) and look for a route to it. The routes go into the route ring
 * (DS:542F) a direction at a time; a squadron sails the first half of one,
 * then looks again. The merchants that bring supplies to ports use the same
 * routes.
 *
 * The searches use WAS.EXE's work arrays at DS:3F3D-4964 (Play.area) in
 * WAS.EXE's order, so that an overflowing queue runs on into the next array
 * as it does there; the opponent's values (DS:604D-607D, from SO2) set how
 * bold and how careful the enemy is.
 */
#include "play_int.h"
#include "gamelog.h"

#include <stdlib.h>

/* the debug log (gamelog.h): only reads the game, never counts work */
#define LOGN(...) GLOG(p, GLOG_NORMAL, __VA_ARGS__)
#define LOGV(...) GLOG(p, GLOG_VERBOSE, __VA_ARGS__)
#define SQN(i)    gamelog_squad(p, (i))
#define CELLN(c)  gamelog_cell(p, (c))

/* the work arrays, as word offsets into Play.area (DS:3F3D) */
enum { A_CAME = 0, A_COST = 200, A_COAST = 400, A_DANGER = 600, A_QUEUE = 800, A_STRENGTH = 900, A_PVALUE = 1100 };

static int16_t *aw(Play *p, int base, int i)
{
    static int16_t outside;
    int k = base + i;
    if (k < 0 || k >= AREA_WORDS) { outside = 0; return &outside; }
    return &p->area[k];
}
#define CAME(i)     (*aw(p, A_CAME, i))
#define COST(i)     (*aw(p, A_COST, i))
#define COAST(i)    (*aw(p, A_COAST, i))
#define DANGER(i)   (*aw(p, A_DANGER, i))
#define QUEUE(i)    (*aw(p, A_QUEUE, i))
#define STRENGTH(i) (*aw(p, A_STRENGTH, i))
#define PVALUE(i)   (*aw(p, A_PVALUE, i))

#define SQ   (p->ai_sq)
#define Q    (&p->sq[p->ai_sq])

static int route_at(const Play *p, int k)   /* DS:542F + k (the byte after the ring is DS:5687) */
{
    if (k >= 0 && k < ROUTE_SIZE) return p->path[PATH_SIZE + k];
    if (k >= ROUTE_SIZE && k < ROUTE_SIZE + MAX_SHIPS) return p->sq[k - ROUTE_SIZE].routed;
    return 0;
}
static void route_set(Play *p, int k, int v) { if (k >= 0 && k < ROUTE_SIZE) p->path[PATH_SIZE + k] = (int8_t)v; }
static int  path_get(const Play *p, int k) { return k >= 0 && k < PATH_SIZE + ROUTE_SIZE ? p->path[k] : 0; }

static int mark_of(const Play *p, int i) { return i >= 0 && i < MAX_SHIPS ? (int8_t)p->sq[i].mark : 0; }
static int speed_word(const Play *p, int i) { return p->sq[i].used ? p->sq[i].speed : NO_DEST; }
static void remove_squadron(Play *p, int i) { p->sq[i].used = false; p->sq[i].speed = NO_DEST; }

/* the search order of directions: DS:[6886] = SO5+12h */
static int dir_order(const Play *p, int k)
{
    int a = 0x12 + 2 * k;
    return (int16_t)(p->ga->so5[a] | (p->ga->so5[a + 1] << 8));
}

/* ------------------------------------------------------------------ squadrons */

/* ovl4_ea05 (has_flagship) */
static bool has_flagship(Play *p, int i)
{
    list_ships(p, i);
    for (int k = 0; k < 3 && p->list[k] != -1; k++) if (p->ship[p->list[k]].type == 3) return true;
    return false;
}

/* ovl4_e96b: a frigate in the squadron being given orders */
static bool frigate_aboard(Play *p)
{
    list_ships(p, SQ);
    for (int k = 0; k < 3 && p->list[k] != -1; k++) if (p->ship[p->list[k]].type == 2) return true;
    return false;
}

/* ovl4_e9b8: a ship of the line (or flagship) in it */
static bool liner_aboard(Play *p)
{
    list_ships(p, SQ);
    for (int k = 0; k < 3 && p->list[k] != -1; k++) if (p->ship[p->list[k]].type != 2) return true;
    return false;
}

/* ovl4_e8c8 (squadron_strength): 6 a frigate, 12 a ship of the line, 18 a
 * flagship, less half its damage; a merchant 1 */
static int strength(Play *p, int i)
{
    if (p->sq[i].mark == 2) return 1;
    list_ships(p, i);
    int s = 0;
    for (int k = 0; k < 3 && p->list[k] != -1; k++) {
        const PShip *sh = &p->ship[p->list[k]];
        s += 6;
        if (sh->type != 2) s += 6;
        if (sh->type == 3) s += 6;
        s -= ((int8_t)sh->sails + (int8_t)sh->hull) / 2;
    }
    return s;
}

/* ovl4_eebf: the enemy's squadrons bound for this cell or in it */
static int bound_for(const Play *p, int cell)
{
    int n = 0;
    for (int i = 0; i < MAX_SHIPS; i++) {
        const PSquad *q = &p->sq[i];
        EV(EV_S40, 1);
        if (!q->used || q->mark == 2 || commandable(p, i)) continue;
        if (q->merchant_dest == cell || play_cell(p, i) == cell) n++;
    }
    return n;
}

/* ovl4_e554: can this port serve? */
static bool port_ok(const Play *p, int cell)
{
    if (cell == -1) return false;
    int t = map_at(p, cell) == 74 ? 74 : 75;
    if (p->port_sup[cell] > 1) return true;
    if (t == 75 && p->ports_supply < 2) return true;
    if (t == 74 && p->ports_repair < 2) return true;
    return false;
}

/* ovl4_b1b5 */
static int iabs(int v) { return v < 0 ? -v : v; }

/* step_distance (ovl4:AF61): steps from x0,y0 to x1,y1 (the larger difference) */
static int step_distance(int x0, int y0, int x1, int y1)
{
    int n = 0;
    while (x0 != x1 || y0 != y1) {
        EV(EV_STEP, 1);
        if (x0 < x1) x0++;
        if (x0 > x1) x0--;
        if (y0 < y1) y0++;
        if (y0 > y1) y0--;
        n++;
    }
    return n;
}

/* ------------------------------------------------------------------ the maps */

/* ovl4_e805 (strength_map): your squadrons' strength negative, theirs
 * positive, by cell; a cell with your flagship -7 */
void enemy_strength_map(Play *p)
{
    EV(EV_BASE, 1931);
    for (int c = 0; c < NUM_CELLS; c++) STRENGTH(c) = 0;
    EV(EV_C200, NUM_CELLS);
    for (int i = 0; i < MAX_SHIPS; i++) {
        EV(EV_S40, 1);
        if (!p->sq[i].used || p->sq[i].mark == 2) continue;
        int c = play_cell(p, i);
        if (has_flagship(p, i) && commandable(p, i)) { STRENGTH(c) = -7; continue; }
        if (STRENGTH(c) == -7) continue;
        int v = strength(p, i);
        if (commandable(p, i)) v = -v;
        STRENGTH(c) += v;
    }
}

/* ovl4_ea51 (port_value_map): the cells beside repair ports, supply ports and
 * crowns, by what they are worth to the enemy */
void enemy_port_values(Play *p)
{
    for (int c = 0; c < NUM_CELLS; c++) PVALUE(c) = 0;
    EV(EV_C200, 2 * NUM_CELLS);
    for (int c = 0; c < NUM_CELLS; c++) {
        int t = p->map[c], col = c % 10;
        if (t != 74 && t != 75 && t != 76 && t != 77) continue;
        int side = t == 77 ? 0 : -1;
        for (int d = 1; d <= 8; d += 2) {
            if (off_map(c, col, d)) continue;
            int n = dir_off(p, d) + c, back = d + 4;
            if (back > 8) back -= 8;
            if (coast_block(p, n, back)) continue;
            if (tile_land(map_at(p, n))) continue;
            if (t == 74) {
                if (repair_port(p, n, true) != -1) PVALUE(n) += p->water_value[0];
            } else if (t == 75) {
                if (supply_port(p, n, true) != -1) PVALUE(n) += p->water_value[1];
            } else if (crown_beside(p, n, side) != -1) {
                if (t == 76) PVALUE(n) = 160;
                else PVALUE(n) += 250;
            }
        }
    }
}

/* ovl4_bd3c (port_side): whose merchant restocks this port: the strength in
 * the cell or the nearest of its neighbours that has any */
int enemy_port_side(const Play *p, int cell)
{
    int v = 0;
    for (int k = 0; k < 9; k++) {
        EV(EV_STEP, 1);
        int i = A_STRENGTH + dir_off(p, k) + cell;
        v = i >= 0 && i < AREA_WORDS ? p->area[i] : 0;
        if (v != 0) break;
    }
    return v < 0 ? -1 : 0;
}

/* ------------------------------------------------------------------ paths and routes */

/* ovl4_b03e: the steps from the squadron to x, y into the path ring after
 * pos, at most limit of them; returns the last position written */
static int path_steps(Play *p, int tx, int ty, int pos, int limit)
{
    int cx = Q->x, cy = Q->y;
    int adx = iabs(cx - tx), ady = iabs(cy - ty), ratio, k = 0;
    if (adx == ady || adx == 0 || ady == 0) ratio = 200;
    else {
        ratio = adx > ady ? ady / (adx - ady) : adx / (ady - adx);
        if (ratio == 0) ratio = 1;
    }
    while (!(cx == tx && cy == ty) && limit != 0) {
        int step = 0;
        if (adx > ady || k < ratio) {
            if (cx < tx) { step++; cx++; }
            if (cx > tx) { step--; cx--; }
        }
        if (ady > adx || k < ratio) {
            if (cy < ty) { step += 80; cy++; }
            if (cy > ty) { step -= 80; cy--; }
        }
        if (++k > ratio) k = 0;
        EV(EV_STEP, 1);
        if (step == 0) continue;
        pos = next_slot(pos);
        if (path_get(p, pos) != 0) {
            if (--pos < 0) pos = 0x5DB;
            break;
        }
        p->path[pos] = (int8_t)step;
        limit--;
    }
    return pos;
}

/* ovl4_d99e: a path for the squadron to x, y (at most limit steps); it sets
 * out at full sail. 0 when the path ring has no room. */
static int sail_to(Play *p, int x, int y, int limit)
{
    int s = find_free_run(p);
    if (s == -1) return 0;
    Q->path_pos = s;
    int e = path_steps(p, x, y, s, limit);
    if (Q->path_pos == e) return 1;
    Q->path_pos = next_slot(Q->path_pos);
    e = next_slot(e);
    if (path_get(p, e) != 0) {
        if (--e < 0) e = 0x5DB;
    }
    p->path[e] = PATH_END;
    Q->countdown = 1;
    Q->has_path = true;
    Q->port_speed = 2;
    Q->speed = 2;
    int was = Q->heading;
    Q->heading = heading_of(path_get(p, Q->path_pos + 2));
    if (Q->heading < 0 || Q->heading > 15) Q->heading = was;
    if (Q->heading < 0 || Q->heading > 15) Q->heading = 0;
    return 1;
}

/* route_free_run (ovl4:B1D0): the start of the longest free run in the route ring, + 1 */
static int route_free_run(const Play *p)
{
    int start = 0, k = 0, best_at = 0, best = 0, run = 0;
    for (;;) {
        while (route_at(p, k) == 0 && k < 0x258) { run++; k++; EV(EV_RING, 1); }
        if (run > best) { best = run; best_at = start; }
        run = 0;
        while (route_at(p, k) != 0) {
            EV(EV_RING, 1);
            k++;
            if (k >= 0x257) break;
        }
        start = k;
        if (k >= 0x257) break;
    }
    return best_at + 1;
}

/* the extra costs of rough and shallow water for the squadron's ships (rules 64B3, 64B9) */
typedef struct { int rough_cost, shallow_cost; bool rough, shallow; } WaterCost;
static WaterCost water_costs(Play *p)
{
    WaterCost w;
    w.rough_cost = p->rules[3] << 5;
    if (p->rules[3] == 0) w.rough_cost = 120;
    w.shallow_cost = p->rules[6] << 5;
    if (p->rules[6] == 0) w.shallow_cost = 120;
    w.rough = frigate_aboard(p) && p->rules[3] != 1;
    w.shallow = liner_aboard(p) && p->rules[6] != 1;
    return w;
}

/* the queue's cheapest cell, taken out (as WAS.EXE, the last cell queued is
 * not looked at while others wait) */
static int queue_take(Play *p)
{
    int at = 0, low = 999;
    EV(EV_QUEUE, 2 * (p->qn > 0 ? p->qn : 0));
    for (int k = 0; k < p->qn; k++) {
        int c = COST(QUEUE(k));
        if (c < low) { low = c; at = k; }
    }
    int cell = QUEUE(at);
    for (int k = at; k < p->qn; k++) QUEUE(k) = QUEUE(k + 1);
    return cell;
}

/* ovl4_db21: a route for the squadron [62FF] to the cell [64DF], avoiding
 * your strength; the first half of it goes into the route ring. 0: none. */
int enemy_route(Play *p)
{
    int start = play_cell(p, SQ);
    WaterCost w = water_costs(p);
    int rough_cost = w.rough_cost;
    if (w.rough && w.shallow) rough_cost /= 2;
    LOGV("ENEMY looks for a route for squadron %d from %s to %s%s%s", SQ, CELLN(start), CELLN(p->ai_target),
         w.rough ? " (keeping out of rough water)" : "", w.shallow ? " (keeping out of shallow water)" : "");
    for (int c = 0; c < NUM_CELLS; c++) { COST(c) = 0; CAME(c) = 0; DANGER(c) = 0; }
    EV(EV_C200, 2 * NUM_CELLS);
    EV(EV_C200, NUM_CELLS);
    for (int c = 0; c < NUM_CELLS; c++) {
        int v = OPP(p, 0x6071) - STRENGTH(c);
        if (v > 0) DANGER(c) += (v >> 1) + 3;
    }
    QUEUE(0) = (int16_t)start;
    p->qn = 1;
    CAME(start) = 9;
    COST(start) = 0;
    bool done = false, found = false;
    int tcol = p->ai_target % 10;
    while (!done) {
        if (--p->qn < 0) break;
        int cur = queue_take(p);
        int col = cur % 10, d;
        /* look first towards the target */
        if (p->ai_target < cur) {
            if (tcol == col) d = 7;
            else if (tcol < col) d = cur - 10 < p->ai_target ? 5 : 6;
            else d = 8;
        } else {
            if (tcol == col) d = 3;
            else if (tcol > col) d = cur + 10 > p->ai_target ? 1 : 2;
            else d = 4;
        }
        if (p->ai_reverse) { d += 4; if (d > 8) d -= 8; }
        int first = d;
        for (int k = 1; k <= 8; k++) {
            EV(EV_NEIGH, 1);
            if (!way_blocked(p, d, cur, col) && !off_map(cur, col, d)) {
                int n = dir_off(p, d) + cur;
                if (COAST(n) != 0x379 && !coast_pass(p, d, n)) {
                    int c = COST(cur) + DANGER(n);
                    if (tile_shallow(map_at(p, n)) && w.shallow) c += w.shallow_cost;
                    if (tile_rough(map_at(p, n)) && w.rough) c += rough_cost;
                    if (CAME(n) == 0 || (CAME(n) < 9 && COST(n) > c)) {
                        CAME(n) = (int16_t)d;
                        QUEUE(p->qn) = (int16_t)n;
                        p->qn++;
                        COST(n) = (int16_t)c;
                    }
                    if (n == p->ai_target) { done = found = true; break; }
                }
            }
            d = dir_order(p, k) + first;
            if (d > 8) d -= 8;
            if (d < 1) d += 8;
        }
    }
    if (!found) {
        LOGN("ENEMY finds no route for squadron %d from %s to %s", SQ, CELLN(start), CELLN(p->ai_target));
        return 0;
    }
    /* back from the target to the start */
    p->qn = 0;
    int cell = p->ai_target;
    for (;;) {
        QUEUE(0) = (int16_t)cell;
        if (cell == start) break;
        cell -= dir_off(p, CAME(cell));
        if (++p->qn > 99) return 0;
        EV(EV_QUEUE, p->qn);
        for (int k = p->qn; k > 0; k--) QUEUE(k) = QUEUE(k - 1);
    }
    /* into the route ring, at most 21 steps */
    int pos = route_free_run(p);
    Q->route_pos = pos;
    int prev = QUEUE(0), n = 0;
    for (int i = 1; i <= p->qn; i++) {
        EV(EV_STEP, 1);
        int delta = QUEUE(i) - prev;
        prev = QUEUE(i);
        int d = 8;
        while (d > 0 && dir_off(p, d) != delta) d--;
        if (pos >= 0x256) { pos = 0x256; break; }
        if (route_at(p, pos) != 0) { pos--; break; }
        if (n > 20) break;
        route_set(p, pos++, d);
        n++;
    }
    route_set(p, pos, PATH_END);
    Q->has_route = 1;
    int8_t all[32];
    int all_n = 0;
    if (gamelog_on(p, GLOG_NORMAL))
        for (int k = Q->route_pos; k < pos && all_n < 32; k++) all[all_n++] = (int8_t)route_at(p, k);
    if (n > 1) {
        int m = Q->route_pos + (n >> 1);
        route_set(p, m++, PATH_END);
        while (m <= pos) route_set(p, m++, 0);
    }
    LOGN("ENEMY route for squadron %d from %s to %s: %d cells (cost %d), sails the first %d: %s", SQ, CELLN(start),
         CELLN(p->ai_target), p->qn, p->cost[p->ai_target], n > 1 ? n >> 1 : n, gamelog_dirs(all, n > 1 ? n >> 1 : n));
    return 1;
}

/* ovl4_e103: the nearest cell beside a port of this kind (74 repair, 75
 * supply) or a crown that can serve, avoiding your strength; -1 none */
static int port_search(Play *p, int kind)
{
    WaterCost w = water_costs(p);
    int rough_cost = w.rough_cost;
    if (w.rough && w.shallow) rough_cost /= 2;
    int start = play_cell(p, SQ);
    for (int c = 0; c < NUM_CELLS; c++) { COST(c) = 0; CAME(c) = 0; }
    EV(EV_C200, 2 * NUM_CELLS);
    EV(EV_C200, NUM_CELLS);
    for (int c = 0; c < NUM_CELLS; c++) {
        int v = -STRENGTH(c);
        if (v > 0) COST(c) += v;
    }
    QUEUE(0) = (int16_t)start;
    p->qn = 1;
    CAME(start) = 9;
    COST(start) = 0;
    int round = 0, best = -1, best_v = 999;
    for (;;) {
        if (--p->qn < 0) return best;
        int cur = queue_take(p);
        int col = cur % 10, d = 1;
        for (int k = 1; k <= 8; k++) {
            EV(EV_NEIGH, 1);
            if (!way_blocked(p, d, cur, col) && !off_map(cur, col, d)) {
                int n = dir_off(p, d) + cur;
                if (COAST(n) != 0x379 && !coast_pass(p, d, n)) {
                    int c = COST(cur);
                    if (tile_shallow(map_at(p, n)) && w.shallow) c += w.shallow_cost;
                    if (tile_rough(map_at(p, n)) && w.rough) c += rough_cost;
                    bool take = CAME(n) == 0 || (CAME(n) < 9 && COST(n) > c);
                    if (take && CAME(n) == 0) {
                        int v = -1;
                        if (crown_beside(p, n, 0) != -1) v = 0;
                        if (crown_beside(p, n, -1) != -1) v = 4;
                        if (kind == 74 && port_ok(p, repair_port(p, n, true))) v = 8;
                        if (kind == 75 && port_ok(p, supply_port(p, n, true))) v = 8;
                        if (v != -1) {
                            v += round;
                            v += count_at(p, n, -1, false) * 4;
                            if (v < best_v) {
                                if (round > 8 && kind < 17) return n;
                                best = n;
                                best_v = v;
                            }
                            if (best <= round) return best;
                        }
                    }
                    if (take) {
                        CAME(n) = (int16_t)d;
                        QUEUE(p->qn) = (int16_t)n;
                        p->qn++;
                        COST(n) = (int16_t)c;
                    }
                }
            }
            d = dir_order(p, k) + 1;
            if (d > 8) d -= 8;
            if (d < 1) d += 8;
        }
        round++;
    }
}

/* ovl4_d799: is the way from cell a to cell b (5 cells at most) blocked by
 * land, or by water the squadron's ships should keep out of? */
static bool way_barred(Play *p, int a, int b)
{
    if (!tile_sea(map_at(p, a))) return true;
    int left = 5;
    int ar = a / 10, ac = a % 10, br = b / 10, bc = b % 10;
    bool rough = frigate_aboard(p) && p->rules[3] != 1;
    bool shallow = liner_aboard(p) && p->rules[6] != 1;
    int dc = iabs(ac - bc), dr = iabs(ar - br), ratio, k = 0;
    if (dc == dr || dc == 0 || dr == 0) ratio = 200;
    else {
        ratio = dc > dr ? dr / (dc - dr) : dc / (dr - dc);
        if (ratio == 0) ratio = 1;
    }
    while (a != b && left != 0) {
        if (dr > dc || k < ratio) {
            if (ar < br) { a += 10; ar++; }
            if (ar > br) { a -= 10; ar--; }
        }
        if (dc > dr || k < ratio) {
            if (ac < bc) { a++; ac++; }
            if (ac > bc) { a--; ac--; }
        }
        if (++k > ratio) k = 0;
        EV(EV_STEP, 1);
        int t = map_at(p, a);
        if (!tile_sea(t)) return true;
        if (rough && tile_rough(t)) return true;
        if (shallow && tile_shallow(t)) return true;
        left--;
    }
    return false;
}

/* ovl4_d5c2: the nearest of your squadrons (or a merchant) worth attacking;
 * [64D7] its distance + 1. ended: the route has run out; pass 1 takes
 * squadrons already in an encounter too. */
static int pick_victim(Play *p, int ended, int pass)
{
    int cell = play_cell(p, SQ);
    p->ai_range = 999;
    int best = 999;
    bool flag = has_flagship(p, SQ);
    int mine = strength(p, SQ) + OPP(p, 0x6065);
    int x = Q->x, y = Q->y;
    for (int k = 0; k < MAX_SHIPS; k++) {
        const PSquad *t = &p->sq[k];
        bool merch = t->mark == 2;
        EV(EV_S40, 1);
        if (!t->used || (t->side == 0 && !merch)) continue;
        if (merch && t->side == 0 && Q->supplies > OPP(p, 0x605D) + 5) continue;
        if (!has_flagship(p, k) && ended == 0) {
            if (strength(p, k) > mine) continue;
            if (flag && rnd(p) < OPP(p, 0x6055) && !merch) continue;
        }
        int kc = play_cell(p, k);
        if (way_barred(p, cell, kc)) continue;
        if (t->partner != 0 && (count_at(p, kc, 0, true) > 2 || pass == 0)) continue;
        int d = step_distance(x, y, t->x, t->y);
        if (t->mark != 0 && d > 1) d >>= 1;
        if (d < p->ai_range && d > 1) { p->ai_range = d; best = k; }
    }
    p->ai_range++;
    return best;
}

/* ovl4_ed83: a squadron of fewer than three ships takes the ships of a
 * squadron of its side close by (join_partner); an emptied squadron is gone */
void enemy_join(Play *p)
{
    list_ships(p, SQ);
    if (p->list[2] != -1) return;
    int mine[3] = { p->list[0], p->list[1], p->list[2] };
    int other = join_partner(p, SQ) - 1;
    if (other < 0) return;
    list_ships(p, other);
    int moved = 0;
    for (int k = 1, j = 0; k < 3; k++) {
        if (mine[k] != -1) continue;
        if (p->list[j] == -1) break;
        p->ship[p->list[j]].sq = SQ;
        j++;
        moved++;
    }
    list_ships(p, other);
    if (p->list[0] == -1) remove_squadron(p, other);
    if (moved) LOGN("ENEMY squadron %d takes %d ship%s from squadron %d close by: now %s%s", SQ, moved, moved > 1 ? "s" : "", other,
                    SQN(SQ), p->sq[other].used ? "" : "; the other squadron is gone");
}

/* ------------------------------------------------------------------ orders */

/* go for the cell [64DF] (ovl4:CFBB) */
static void route_to_target(Play *p)
{
    if (enemy_route(p)) {
        Q->routed = 1;
        if (STRENGTH(p->ai_target) < 0) Q->routed = 2;
    }
}
static void set_target(Play *p, int cell)
{
    p->ai_target = cell;
    Q->merchant_dest = cell;
    route_to_target(p);
}

/* ovl4_caea: a target for the squadron: off the land, to a port for supplies
 * or repairs, or the most tempting cell on the map: your crown, a flagship,
 * your squadrons where they are weak */
static void choose_target(Play *p)
{
    int mycell = play_cell(p, SQ);
    if ((uint16_t)COAST(mycell) == 0x379) {
        /* in a cell that is all land: out to a sea cell beside it */
        int my = cell_mid_y(p, mycell), mx = cell_mid_x(p, mycell);
        int ax = Q->x - mx, ay = Q->y - my;
        bool west = false, north = false;
        if (ax < 0) { ax = -ax; west = true; }
        if (ay < 0) { ay = -ay; north = true; }
        int first = ax > ay ? (west ? 4 : 8) : (north ? 6 : 2);
        int col = mycell % 10, n = 0;
        for (int k = 0; k < 8; k++) {
            n = k + first;
            if (n > 8) n -= 8;
            if ((n > 3 && n < 7 && col == 0) || ((n > 7 || n < 3) && col == 9)) continue;
            n = dir_off(p, n) + mycell;
            if (n < 0 || n > 199) continue;
            if (tile_sea(p->map[n])) {
                LOGN("ENEMY %s is in a cell that is all land: out to the sea at %s", SQN(SQ), CELLN(n));
                set_target(p, n);
                return;
            }
        }
        LOGN("ENEMY %s is in a cell that is all land: tries %s", SQN(SQ), CELLN(n));
        set_target(p, n);
        return;
    }
    bool hungry = Q->supplies < OPP(p, 0x605D);
    if (hungry && p->ports_supply != 0) {
        int t = port_search(p, 75);
        if (t != -1) {
            LOGN("ENEMY sends %s for supplies to %s (supplies %d under 605D %d)", SQN(SQ), CELLN(t), Q->supplies, OPP(p, 0x605D));
            set_target(p, t);
            return;
        }
        LOGV("ENEMY %s is hungry but finds no port to serve it", SQN(SQ));
    }
    if (worst_damage(p, SQ) > OPP(p, 0x605F) && !hungry && p->ports_repair != 0) {
        int t = port_search(p, 74);
        if (t != -1) {
            LOGN("ENEMY sends %s for repairs to %s (worst damage %d over 605F %d)", SQN(SQ), CELLN(t), gamelog_worst(p, SQ),
                 OPP(p, 0x605F));
            set_target(p, t);
            return;
        }
        LOGV("ENEMY %s is damaged but finds no port to repair it", SQN(SQ));
    }
    if (bound_for(p, p->crown_threat) >= OPP(p, 0x604D) && rnd(p) < 60) {
        if (p->crown_threat != -1) LOGN("ENEMY drops its standing target %s: enough squadrons are bound for it (604D %d; chance 60/128)",
             CELLN(p->crown_threat), OPP(p, 0x604D));
        p->ai_dropped = p->crown_threat;
        p->crown_threat = -1;
    }
    if (p->crown_threat == -1) {
        int best = -860;
        bool special = false;                /* the log: your crown or flagship chosen outright */
        for (int c = 0; c < NUM_CELLS; c++) {
            EV(EV_C200, 1);
            if (tile_land(p->map[c]) || c == p->ai_dropped) continue;
            if (STRENGTH(c) == 0 && PVALUE(c) == 0 && rnd(p) < 100) continue;
            if (PVALUE(c) == 160 && bound_for(p, c) < OPP(p, 0x6073)) {
                LOGN("ENEMY sends %s to guard its crown at %s (fewer than 6073 %d squadrons bound for it)", SQN(SQ), CELLN(c),
                     OPP(p, 0x6073));
                set_target(p, c);
                return;
            }
            if (PVALUE(c) >= 250 && rnd(p) < OPP(p, 0x6075) && p->ports_supply == 0 && p->ports_repair == 0) {
                p->crown_threat = c;
                special = true;
                LOGN("ENEMY new standing target: your crown, beside %s (chance 6075 %d/128, neither side has ports)", CELLN(c),
                     OPP(p, 0x6075));
                break;
            }
            if (STRENGTH(c) == -7 && rnd(p) < OPP(p, 0x6075) && p->ports_supply == 0 && p->ports_repair == 0) {
                p->crown_threat = c;
                special = true;
                LOGN("ENEMY new standing target: your flagship at %s (chance 6075 %d/128, neither side has ports)", CELLN(c),
                     OPP(p, 0x6075));
                break;
            }
            if (STRENGTH(c) > 0 && rnd(p) < 94) continue;
            int v = 0;
            if (STRENGTH(c) < 0) v = STRENGTH(mycell) + STRENGTH(c);
            if (OPP(p, 0x6071) > v && rnd(p) < 90) continue;
            if (rnd(p) < 30 || PVALUE(c) > 200) v += PVALUE(c) / 2;
            if (rnd(p) < OPP(p, 0x6061)) v -= rnd(p);
            LOGV("ENEMY   %s: strength %d, port value %d: scores %d%s", CELLN(c), STRENGTH(c), PVALUE(c), v, v > best ? " (best so far)" : "");
            if (v > best) { best = v; p->crown_threat = c; }
        }
        if (p->crown_threat != -1 && !special)
            LOGN("ENEMY new standing target for %s: %s, the best score %d (your strength there and its own against 6071 %d, "
                 "half the port value at times, randomness 6061 %d)", SQN(SQ), CELLN(p->crown_threat), best, OPP(p, 0x6071), OPP(p, 0x6061));
        if (p->crown_threat == mycell) { LOGV("ENEMY %s stays: the best target is its own cell", SQN(SQ)); return; }
        if (p->crown_threat == -1) p->ai_dropped = -1;
        if (p->crown_threat == -1 && rnd(p) < 60 && p->ports_supply != 0) {
            p->crown_threat = port_search(p, 75);
            if (p->crown_threat != -1) LOGN("ENEMY finds nothing worth going for: a supply port at %s instead", CELLN(p->crown_threat));
        }
        if (p->crown_threat == -1 && rnd(p) < 60 && p->ports_repair != 0) {
            p->crown_threat = port_search(p, 74);
            if (p->crown_threat != -1) LOGN("ENEMY finds nothing worth going for: a repair port at %s instead", CELLN(p->crown_threat));
        }
        if (p->crown_threat == -1) { LOGV("ENEMY finds nothing worth going for: %s stays", SQN(SQ)); return; }
        p->ai_reverse = 1;
        p->merchant_lost = 1;
        if (rnd(p) < 64) p->ai_reverse = 0;
    } else if (rnd(p) < 25) p->merchant_lost = 0;
    LOGN("ENEMY sends %s to its standing target %s", SQN(SQ), CELLN(p->crown_threat));
    set_target(p, p->crown_threat);
}

/* ovl4_c88a: an anchored squadron without a route: stay (in port to take on
 * supplies or repair, beside its crown, in an encounter's reach), or choose
 * a target */
static void new_orders(Play *p)
{
    int cell = play_cell(p, SQ);
    bool crown = crown_beside(p, cell, -1) != -1 || crown_beside(p, cell, 0) != -1;
    if (rnd(p) > OPP(p, 0x6057) >> 3 && p->merchant_lost == 0 && PVALUE(cell) != 0) {
        LOGV("ENEMY %s simply stays by the port or crown (random over 6057/8 = %d)", SQN(SQ), OPP(p, 0x6057) >> 3);
        return;
    }
    if (PVALUE(cell) != 0 && OPP(p, 0x6057) != 115) {
        bool flag = has_flagship(p, SQ) && (rnd(p) > OPP(p, 0x6057) >> 3 || rnd(p) > 5);
        int dmg = worst_damage(p, SQ);
        int port = supply_port(p, cell, true);
        if (port != -1 || crown) {
            if (Q->supplies < 18) {
                if (port_ok(p, port) || crown) { LOGV("ENEMY %s stays to take on supplies (%d)", SQN(SQ), Q->supplies); return; }
            } else if (flag && OPP(p, 0x605F) >= dmg) { LOGV("ENEMY %s stays: a flagship squadron in port", SQN(SQ)); return; }
        }
        port = repair_port(p, cell, true);
        if (port != -1 || crown) {
            if (dmg != 0) {
                if (Q->supplies > 8 && (port_ok(p, port) || crown)) { LOGV("ENEMY %s stays for repairs (damage %d)", SQN(SQ), dmg); return; }
            } else if (flag && Q->supplies >= OPP(p, 0x605D)) { LOGV("ENEMY %s stays: a flagship squadron in port", SQN(SQ)); return; }
        }
        for (int k = 0; k < MAX_SHIPS; k++) {
            const PSquad *t = &p->sq[k];
            EV(EV_S40, 1);
            if (t->side == 0 || !t->used || t->mark == 2) continue;
            if (close_to(p, SQ, k)) { LOGV("ENEMY %s stays in port: your squadron %d is close", SQN(SQ), k); return; }
        }
    }
    if (crown_beside(p, cell, -1) != -1 && count_at(p, cell, 0, false) <= OPP(p, 0x6073)) {
        LOGV("ENEMY %s guards its crown (no more than 6073 %d of its squadrons there)", SQN(SQ), OPP(p, 0x6073));
        return;
    }
    if (cell != p->crown_threat) choose_target(p);
    else LOGV("ENEMY %s stays: it is at its standing target", SQN(SQ));
}

/* ovl4_cfef: an anchored squadron on a route: to the middle of its cell,
 * then attack a squadron near enough, or on along the route, or it has
 * arrived: a merchant unloads, a warship joins others and puts into port */
static void follow_route(Play *p)
{
    int cell = play_cell(p, SQ);
    p->ai_range = 5;
    int my = cell_mid_y(p, cell), mx = cell_mid_x(p, cell);
    if (!tile_land(map_at(p, cell)) && (Q->x != mx || Q->y != my)) {
        LOGV("ENEMY %s sails to the middle of its cell", SQN(SQ));
        sail_to(p, mx, my, p->ai_range);
        return;
    }
    p->ai_range = 999;
    int ended = route_at(p, Q->route_pos) == PATH_END;
    if (Q->mark != 2 && crown_beside(p, Q->merchant_dest, -1) == -1 && crown_beside(p, Q->merchant_dest, 0) == -1) {
        for (int pass = 0; pass < 2; pass++) {
            int t = pick_victim(p, ended, pass);
            if ((OPP(p, 0x6063) << 3) < p->ai_range) continue;
            if (Q->supplies < OPP(p, 0x605D) && p->ports_supply != 0) continue;
            if (worst_damage(p, SQ) > OPP(p, 0x605F) && p->ports_repair == 0) continue;
            if (mark_of(p, t) != 0) p->ai_range <<= 1;
            if (p->ai_range > 8) p->ai_range = 8;
            int tx = t < MAX_SHIPS ? p->sq[t].x : 0, ty = t < MAX_SHIPS ? p->sq[t].y : 0;
            LOGN("ENEMY %s attacks %s, %d steps away (strength %d + 6065 %d against %d; attacks within 6063 x 8 = %d)", SQN(SQ),
                 t < MAX_SHIPS ? SQN(t) : "nothing", p->ai_range, gamelog_strength(p, SQ), OPP(p, 0x6065),
                 gamelog_strength(p, t < MAX_SHIPS ? t : 0), OPP(p, 0x6063) << 3);
            clear_paths(p, SQ);
            route_set(p, Q->route_pos, PATH_END);
            int v = sail_to(p, tx, ty, p->ai_range) ? 2 : 0;
            Q->port_speed = v;
            Q->speed = v;
            Q->has_route = v;
            return;
        }
    }
    p->ai_range = 5;
    int d = route_at(p, Q->route_pos);
    const char *why = "stops at";           /* the log */
    for (;;) {
        route_set(p, Q->route_pos, 0);
        if (d != PATH_END && d != 0) {
            /* the next leg of the route */
            int n = dir_off(p, d) + cell;
            p->ai_range = 30;
            if (!sail_to(p, cell_mid_x(p, n), cell_mid_y(p, n), p->ai_range)) {
                route_set(p, Q->route_pos, d);
                LOGV("ENEMY %s waits: the path ring is full", SQN(SQ));
            } else {
                Q->route_pos++;
                LOGV("ENEMY %s sails on along its route to %s", SQN(SQ), CELLN(n));
            }
            return;
        }
        int dest = Q->merchant_dest;
        if (cell == dest) { why = "has arrived at"; break; }
        if (!(STRENGTH(dest) < 0 || Q->mark == 2) && Q->routed == 2) { why = "finds your squadrons gone from"; break; }
        if (Q->supplies < OPP(p, 0x605D) && p->ports_supply != 0) { why = "is hungry (605D): gives up going for"; break; }
        if (worst_damage(p, SQ) > OPP(p, 0x605F) && p->ports_repair != 0) { why = "is damaged (605F): gives up going for"; break; }
        p->ai_target = dest;
        if (!enemy_route(p)) { why = "can't get to"; break; }
        d = route_at(p, Q->route_pos);
        if (d == PATH_END || d == 0) { why = "can't get further toward"; break; }
    }
    LOGN("ENEMY %s %s %s", SQN(SQ), why, CELLN(Q->merchant_dest));
    /* arrived (or giving up) */
    redraw_work(p);
    Q->port_speed = 0;
    Q->speed = 0;
    Q->has_route = 0;
    Q->routed = 0;
    int c2 = crown_beside(p, cell, -1);
    if (c2 == -1) c2 = crown_beside(p, cell, 0);
    if (Q->mark == 2) {
        /* a merchant unloads: the ports beside are full again */
        int s = supply_port(p, cell, false);
        if (s == -1) s = c2;
        if (s != -1) {
            if (p->port_sup[s] == 0) invert(p, s);
            p->port_sup[s] = p->rules[1] << 1;
        }
        int r = repair_port(p, cell, false);
        if (r == -1) r = c2;
        if (r != -1) {
            if (p->port_sup[r] == 0) invert(p, r);
            p->port_sup[r] = p->rules[1];
        }
        LOGN("ENEMY merchant %d unloads: supply port %s, repair port %s are full again", SQ, s == -1 ? "none" : CELLN(s),
             r == -1 ? "none" : CELLN(r));
        Q->mark = 0;
        remove_squadron(p, SQ);
        return;
    }
    enemy_join(p);
    int r = rnd(p);
    if (r < 43) Q->x++;
    else if (r < 86) Q->x--;
    r = rnd(p);
    if (r < 43) Q->y++;
    else if (r < 86) Q->y--;
    if (cell == p->crown_threat) {
        LOGN("ENEMY has reached its standing target %s: it will choose a new one", CELLN(cell));
        p->ai_dropped = p->crown_threat;
        p->crown_threat = -1;
        p->merchant_lost = 0;
    }
    enemy_join(p);
    port_call(p, cell, Q->side);
}

/* ovl4_c79a: the next of the enemy's anchored squadrons (or anyone's
 * merchant) is given orders */
void enemy_turn(Play *p)
{
    EV(EV_BASE, 142);
    int was = p->ai_done;
    if (++p->ai_done >= MAX_SHIPS) p->ai_done = 0;
    while (p->ai_done != was) {
        const PSquad *q = &p->sq[p->ai_done];
        EV(EV_S40, 1);
        if (!(q->side == -1 && q->mark != 2) && speed_word(p, p->ai_done) == 0) break;
        if (++p->ai_done >= MAX_SHIPS) p->ai_done = 0;
    }
    const PSquad *q = &p->sq[p->ai_done];
    if (speed_word(p, p->ai_done) != 0 || q->partner != 0) return;
    if (q->side == -1 && q->mark != 2) return;
    p->ai_sq = p->ai_done;
    LOGV("ENEMY looks at %s (%s)", SQN(SQ), Q->has_route ? "on a route" : "no orders");
    /* ovl4_c862 */
    if (Q->has_route) {
        if (p->mode != PM_TRACE) follow_route(p);
    } else new_orders(p);
}
