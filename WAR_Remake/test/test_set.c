/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test_set.c: games saved by WARET.EXE, the 16-colour program (SET), as the
 * remake carries them on. Needs saves made in DOSBox-X
 * (WAR_Analysis/tools/dosbox wsaveset.sh):
 *   S-START, SET-START: WAR.EXE's S and WARET.EXE's SET saved as the first
 *     campaign reaches the map, so both hold the same game
 *   SET-PLAY: squad 16 traced north-west, time fast, some 25 s on
 *   SET-MARK: squad 16 on a traced path with its mark
 * test_set GAME S-START SET-START SET-PLAY SET-MARK DATA-FOLDER
 */
#include "../src/menu.h"
#include "../src/savegame.h"
#include "../src/sound.h"
#include <SDL3/SDL.h>

static int fails;
#define CHECK(c, ...) do { if (c) SDL_Log("ok   " __VA_ARGS__); else { SDL_Log("FAIL " __VA_ARGS__); fails++; } } while (0)

static GameAssets ga;
static Campaign camps[NUM_CAMPAIGNS];

static bool import(const char *file, Play *p, Campaign *c)
{
    size_t n = 0;
    uint8_t *d = SDL_LoadFile(file, &n);
    bool ok = d && savegame_import(p, c, &ga, d, n, 7);
    SDL_free(d);
    return ok;
}

static void copy_file(const char *from, const char *dir, const char *name)
{
    size_t n = 0;
    void *d = SDL_LoadFile(from, &n);
    char to[1200];
    SDL_snprintf(to, sizeof to, "%s/%s", dir, name);
    SDL_SaveFile(to, d, n);
    SDL_free(d);
}

int main(int argc, char **argv)
{
    if (argc < 7) { SDL_Log("test_set GAME S-START SET-START SET-PLAY SET-MARK DATA-FOLDER"); return 2; }
    char err[256];
    sound_init_offline();
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("%s", err); return 1; }
    campaigns_load(camps, argv[1]);

    /* the same game saved by both programs */
    static Play ps, pe; static Campaign cs, ce;
    CHECK(import(argv[2], &ps, &cs), "WAR.EXE's S");
    CHECK(import(argv[3], &pe, &ce), "WARET.EXE's SET (%d bytes)", ORIGINAL_SET_SIZE);
    CHECK(!SDL_memcmp(cs.map, ce.map, sizeof cs.map) && !SDL_memcmp(cs.armies, ce.armies, sizeof cs.armies), "the same map and armies");
    CHECK(cs.opponent == ce.opponent && !SDL_memcmp(cs.rules, ce.rules, sizeof cs.rules), "the same opponent (%d) and rules", ce.opponent);
    CHECK(!SDL_memcmp(ps.opp, pe.opp, sizeof ps.opp), "the opponent's values");
    CHECK(!SDL_memcmp(ps.cell_cost, pe.cell_cost, sizeof ps.cell_cost), "the enemy's cell costs");
    CHECK(ps.view_top == pe.view_top && ps.cx == pe.cx && ps.cy == pe.cy, "the view (%d) and the cursor (%d,%d)", pe.view_top, pe.cx, pe.cy);
    bool same = true;
    for (int i = 0; i < MAX_SQUADS; i++) {
        const PSquad *a = &ps.sq[i], *b = &pe.sq[i];
        if (a->used != b->used || (a->used && (a->friendly != b->friendly || a->men != b->men || a->formation != b->formation))) same = false;
        if (a->used && a->friendly && (a->x != b->x || a->y != b->y || a->condition != b->condition || a->food != b->food)) same = false;
    }
    CHECK(same, "the squads (yours where they started)");
    for (int i = 0; i < MAX_FLAGS; i++) same = same && ps.flag[i].present == pe.flag[i].present && ps.flag[i].x == pe.flag[i].x;
    CHECK(same, "the flags");

    /* a game played on */
    static Play p; static Campaign c;
    CHECK(import(argv[4], &p, &c), "a game played on");
    const PSquad *q = &p.sq[16];
    CHECK(q->used && q->friendly && q->x < 40 && q->y < 180 && q->archers == 14, "squad 16 gone north-west (%d,%d), 14 archers", q->x, q->y);
    int x16 = q->x, y16 = q->y;
    CHECK(p.time_div == 4 && p.sel == 16 && p.view_top == 109, "time fast (%d), squad 16 chosen (%d), the view (%d)", p.time_div, p.sel, p.view_top);
    int moving = 0;
    for (int i = 0; i < MAX_SQUADS; i++) moving += p.sq[i].used && !p.sq[i].friendly && (p.sq[i].moving || p.sq[i].on_route);
    CHECK(moving > 0, "enemy squads on the move (%d)", moving);
    p.fight_delay = 400;
    play_resume(&p);
    long f = 0;
    for (; f < 20000 && p.result == PLAY_RUNNING; f++) {
        play_frame(&p, 0);
        if (p.mode == PM_PROMPT) play_key(&p, p.ended ? PK_ENTER : PK_N, false);
    }
    CHECK(f > 0, "played on %ld iterations (result %d)", f, p.result);

    CHECK(import(argv[5], &p, &c), "a game with a mark");
    CHECK(p.sq[16].used && p.sq[16].has_path && !p.sq[16].marked, "squad 16 on its path, the mark dropped as WARET.EXE does");

    /* on a data disk: SET alone, then both (the newer one wins) */
    char dir[1100];
    SDL_snprintf(dir, sizeof dir, "%s/Data Disks/Data Disk 1", argv[6]);
    SDL_CreateDirectory(dir);
    char root[1100];
    SDL_snprintf(root, sizeof root, "%s/", argv[6]);
    copy_file(argv[4], dir, "SET");
    Menu *m = menu_create(&ga, camps, root);
    CHECK(menu_has_saved_game(m), "a data disk with SET has a saved game");
    static Play pm; static Campaign cm;
    CHECK(menu_load_saved_game(m, &pm, &cm) && pm.sq[16].x == x16 && pm.sq[16].y == y16 && cm.title[0], "it loads, title '%s'", cm.title);
    SDL_Delay(1100);
    copy_file(argv[2], dir, "S");                       /* S saved later */
    CHECK(menu_load_saved_game(m, &pm, &cm) && (pm.sq[16].x != x16 || pm.sq[16].y != y16), "S saved later is the game");
    SDL_Delay(1100);
    copy_file(argv[5], dir, "SET");                     /* SET saved later */
    CHECK(menu_load_saved_game(m, &pm, &cm) && pm.sq[16].has_path, "SET saved later is the game");
    menu_destroy(m);

    SDL_Log("%d failed", fails);
    return fails != 0;
}
