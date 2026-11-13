/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/fuzz_editor.c: step 7, random keys and typing at random moments
 * through the campaign menu and the editors (under AddressSanitizer): new
 * campaigns made, changed and removed, the map maker, the ships editor,
 * the scroll editor; every page drawn in both modes. Then every S file and
 * SITLES on the data disk must be whole and load.
 *
 * Usage: fuzz_editor GAME-FOLDER WORK-FOLDER [KEYS] [SEED]
 */
#include <SDL3/SDL.h>

#include "../src/assets.h"
#include "../src/campaign.h"
#include "../src/menu.h"
#include "../src/play.h"

static uint32_t r = 1;
static int rnd(int n) { r = r * 1103515245u + 12345u; return (int)((r >> 16) % (unsigned)n); }

int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("usage: fuzz_editor GAME-FOLDER WORK-FOLDER [KEYS] [SEED]"); return 2; }
    int keys = argc > 3 ? SDL_atoi(argv[3]) : 20000;
    if (argc > 4) r = (uint32_t)SDL_atoi(argv[4]);
    static GameAssets ga;
    static Campaign camps[NUM_CAMPAIGNS];
    static Screen scr;
    char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("assets: %s", err); return 2; }
    campaigns_load(camps, argv[1]);
    char data[1200];
    SDL_snprintf(data, sizeof data, "%s/", argv[2]);
    SDL_CreateDirectory(data);
    Menu *m = menu_create(&ga, camps, data);
    static const int KEYS[] = {
        PK_ENTER, PK_ENTER, PK_ENTER, PK_KP_PLUS, PK_KP_MINUS, PK_EQUALS, PK_MINUS, PK_ESC, PK_SPACE, PK_BACKSPACE,
        PK_UP, PK_DOWN, PK_LEFT, PK_RIGHT, PK_HOME, PK_END, PK_PGUP, PK_PGDN, PK_UP, PK_DOWN, PK_LEFT, PK_RIGHT,
        PK_Y, PK_Y, PK_N, PK_S, PK_S, PK_F, PK_I, 0x22 /* G */, PK_P, 0x13 /* R */, PK_T, 0x23 /* H */, PK_E, PK_DELETE,
    };
    const int nkeys = (int)SDL_arraysize(KEYS);
    int games = 0, texts = 0;
    menu_start(m);
    for (int k = 0; k < keys; k++) {
        int frames = rnd(4) == 0 ? rnd(300) : rnd(20);
        for (int f = 0; f < frames; f++) {
            menu_run(m, 1e6 / 60);
            MenuAction a = menu_action(m);
            if (a == MENU_START_GAME || a == MENU_DEMO || a == MENU_RESUME_GAME || a == MENU_QUIT) {
                if (a == MENU_START_GAME) games++;
                menu_title(m);
            }
        }
        if (menu_wants_text(m) && rnd(3)) {             /* typing */
            char t[8];
            int n = 1 + rnd(6);
            for (int i = 0; i < n; i++) t[i] = (char)(32 + rnd(95));
            t[n] = 0;
            menu_text(m, t);
            texts++;
        } else {
            int key = rnd(8) == 0 ? 0x10 + rnd(0x2B) : KEYS[rnd(nkeys)];   /* any letter now and then */
            menu_key(m, key);
            if (rnd(3)) menu_key_up(m);
        }
        if (k % 40 == 0) { menu_draw(m, &scr, MODE_CGA); menu_draw(m, &scr, MODE_16); }
    }
    menu_destroy(m);
    /* what the editors wrote: SITLES and the S files it names, whole */
    int bad = 0, files = 0;
    char disk[1300];
    SDL_snprintf(disk, sizeof disk, "%sData Disks/Data Disk 1/", data);
    size_t size = 0;
    uint8_t *t = assets_load_file(disk, "SITLES", &size);
    if (t && size != 876) { SDL_Log("SITLES is %d bytes", (int)size); bad++; }
    for (int i = 0; t && i < 12; i++) {
        char name[4] = { (char)t[i * 3], (char)t[i * 3 + 1], 0, 0 };
        size_t n = 0;
        uint8_t *d = assets_load_file(disk, name, &n);
        if (!d) continue;
        Campaign c;
        files++;
        if (n != 2180 || !campaign_parse(&c, name, d, n)) { SDL_Log("%s is %d bytes", name, (int)n); bad++; }
        SDL_free(d);
    }
    SDL_free(t);
    SDL_Log("fuzz_editor: %d keys, %d typed; %d games; %d S files, %d bad", keys, texts, games, files, bad);
    assets_free(&ga);
    return bad != 0;
}
