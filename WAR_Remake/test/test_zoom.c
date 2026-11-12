/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/test_zoom.c: a test or debugging aid for the remake (see README.md here)
 */
/* Test harness: a Zoom battle (overlay 7) with screenshots. */
#define BATTLE_TRACE(...) SDL_Log(__VA_ARGS__)
#include "../src/play.c"
#include "../src/battle.c"
static GameAssets ga; static Campaign camps[NUM_CAMPAIGNS]; static Play play; static Screen scr;
static Battle bt;
static uint32_t rgb[SCREEN_W * SCREEN_H]; static const char *outdir;
static void shot(const char *name, GfxMode m)
{
    battle_draw(&bt, &scr, m); screen_to_xrgb(&scr, rgb);
    SDL_Surface *s = SDL_CreateSurfaceFrom(SCREEN_W, SCREEN_H, SDL_PIXELFORMAT_XRGB8888, rgb, SCREEN_W * 4);
    char p[512]; SDL_snprintf(p, sizeof p, "%s/%s.bmp", outdir, name); SDL_SaveBMP(s, p); SDL_DestroySurface(s);
}
static int alive(int side) { int n = 0; for (int i = 0; i < MAX_MEN; i++) if (bt.addr[i] != -1 && bt.stat[i] != DEAD && bt.side[i] == side) n++; return n; }
typedef struct { int frame, key; } K;
static void run(int camp, int fr, int en, uint32_t seed, const char *tag, const K *keys)
{
    play_start(&play, &camps[camp], &ga, seed);
    PSquad *a = &play.sq[fr], *e = &play.sq[en];
    a->x = e->x + 1; a->y = e->y + 3;
    play.time_div = 1;
    for (int f = 0; f < 50 && !a->fighting; f++) play_frame(&play, 0);
    SDL_Log("%s: sq%d (a%d k%d b%d) v sq%d (a%d k%d b%d) fighting %d clock %d", tag, fr, a->archers, a->knights, a->barbarians,
            en, e->archers, e->knights, e->barbarians, a->fighting, a->clock);
    if (!a->fighting) { SDL_Log("  no encounter"); return; }
    play.sel = fr; play.mode = PM_SQUAD;
    play.cx = a->x + 1; play.cy = a->y; check_over(&play, 1);
    play_key(&play, PK_Z, false);
    play_frame(&play, 0);
    SDL_Log("  zoom_squad %d (mode %d clock %d)", play.zoom_squad, play.mode, a->clock);
    if (play.zoom_squad < 0) play.zoom_squad = fr;
    if (!battle_start(&bt, &play, play.zoom_squad)) { SDL_Log("  battle_start failed"); return; }
    SDL_Log("  bg %d skill %d/%d  formation %d/%d  men %d v %d", bt.bg, bt.skill_me, bt.skill_en,
            play.sq[bt.me].formation, play.sq[bt.en].formation, alive(FRIEND), alive(ENEMY));
    char n[64];
    SDL_snprintf(n, sizeof n, "%s_0start_cga", tag); shot(n, MODE_CGA);
    SDL_snprintf(n, sizeof n, "%s_0start_vga", tag); shot(n, MODE_VGA);
    int f;
    for (f = 0; f < 3000 && !bt.done; f++) {
        for (const K *k = keys; k->key; k++) if (k->frame == f) battle_key(&bt, k->key);
        battle_frame(&bt);
        if (f == 30 || f == 60 || f == 100 || f == 150) {
            SDL_snprintf(n, sizeof n, "%s_f%03d", tag, f); shot(n, MODE_VGA);
            SDL_Log("  f%d: %d v %d, arrows %d %d %d, rows [%s] [%s]", f, alive(FRIEND), alive(ENEMY), bt.ax[0], bt.ax[1], bt.ax[2], bt.text[0], bt.text[1]);
        }
        if (bt.endtimer == 1) { SDL_snprintf(n, sizeof n, "%s_zend", tag); shot(n, MODE_VGA); }
    }
    SDL_Log("  done after %d frames: lost %d / %d, retreat %d, rows [%s] [%s]", f, bt.lost_me, bt.lost_en, bt.retreat, bt.text[0], bt.text[1]);
    battle_finish(&bt);
    SDL_Log("  after: me men %d cond %d at %d,%d  en men %d cond %d  mode %d fighting %d/%d", a->men, a->condition, a->x, a->y,
            e->men, e->condition, play.mode, a->fighting, e->fighting);
}
int main(int argc, char **argv)
{
    outdir = argv[1]; char err[256];
    if (!assets_load(&ga, argv[2], err, sizeof err)) return 1;
    campaigns_load(camps, argv[2]);
    SDL_Log("have_battle %d", ga.have_battle);
    (void)argc;
    static const K idle[] = { { 25, PK_A }, { 0, 0 } };
    static const K shoot[] = { { 25, PK_A }, { 27, PK_A }, { 29, PK_A }, { 0, 0 } };   /* Attack; Archer, Attack */
    static const K squad[] = { { 5, PK_A }, { 7, PK_S }, { 9, PK_A }, { 0, 0 } };      /* Attack; Squad, Attack */
    static const K run_away[] = { { 40, PK_R }, { 0, 0 } };
    run(0, 18, 6, 7, "z1", idle);
    run(0, 18, 6, 7, "z2", shoot);
    run(0, 16, 13, 9, "z3", squad);
    run(0, 18, 6, 11, "z4", run_away);
    for (int c = 1; c < NUM_CAMPAIGNS; c += 2) {      /* other campaigns: first squads of each side */
        play_start(&play, &camps[c], &ga, 1);
        int fr = -1, en = -1;
        for (int i = 0; i < MAX_SQUADS; i++) {
            if (!play.sq[i].used || play.sq[i].knights + play.sq[i].archers + play.sq[i].barbarians == 0) continue;
            if (play.sq[i].friendly && fr < 0) fr = i;
            if (!play.sq[i].friendly && en < 0) en = i;
        }
        char tag[8]; SDL_snprintf(tag, sizeof tag, "c%d", c);
        if (fr >= 0 && en >= 0) run(c, fr, en, 3 + c, tag, squad);
    }
    assets_free(&ga);
    return 0;
}
