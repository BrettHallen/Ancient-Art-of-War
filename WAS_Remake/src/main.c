/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * main.c: the window, the asset viewer (step 1) and campaign play (steps 2
 * and 3).
 *
 * Everything is drawn from the player's own copy of the original game, in
 * the CGA colours (WAS.EXE) or the 16 colours of the Tandy 1000 version
 * (WAST.DAT).
 *
 * Viewer keys:  Left/Right, Space  previous/next screen    Tab   CGA / 16 colours / Hercules
 *        Up/Down, -/+, mouse wheel   scroll a campaign map
 *        Shift+-/+, Home/End         top / bottom of the map
 *        Enter       play the campaign shown
 *        I           on a map: its story, then its ships, then the map again
 *        F2 or A     4:3 or square pixels    H     show/hide the caption
 *        F, Alt+Enter, Cmd+Enter  full screen  O   choose the game folder
 *        Esc         quit
 *
 * Playing: the original keys (arrows or keypad move the marker, -/+ scroll,
 *        M Move (trace, M marks, C cancels), I Info (S speed), D Detach,
 *        J Join (-/+ mark, S switch, E end), E End, T Time, F Fleet,
 *        P Pause, S Surrender, C Clear, Y/N answers, Enter at the end), plus
 *        Tab CGA/16 colours/Hercules, F2 aspect, Alt/Cmd+Enter full screen,
 *        Esc leave the game.
 *
 * Usage: AncientArtOfWarAtSea [HERC] [game-folder]   (HERC: start on a Hercules card, as WAS HERC)
 *        AncientArtOfWarAtSea --screenshots OUTDIR [game-folder]   (write BMPs, no window)
 */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "assets.h"
#include "battle.h"
#include "campaign.h"
#include "gfx.h"
#include "icon.h"
#include "play.h"
#include "viewer.h"

#define APP_ORG   "AncientArtOfWarAtSea"
#define APP_NAME  "Remake"
#define APP_TITLE "The Ancient Art of War at Sea"

typedef struct {
    GameAssets    ga;
    bool          have_assets;
    Campaign      campaigns[NUM_CAMPAIGNS];
    Viewer        viewer;
    bool          playing;
    Campaign      game;          /* the campaign being played */
    Play          play;
    Battle       *battle;        /* the sea battle (Zoom, Practice), or NULL */
    Uint64        battle_ns;     /* the battle's last run */
    int           tapped;        /* marker key pressed since the last frame */
    bool          aspect_43;     /* true: 4:3 like a CRT, false: square pixels */
    Screen        screen;
    uint32_t      rgb[HERC_W * HERC_H];   /* the screen, 320x200 or 720x348 (Hercules) */
    SDL_Window   *win;
    SDL_Renderer *ren;
    SDL_Texture  *tex;           /* 320x200 game screen */
    SDL_Texture  *tex_herc;      /* 720x348 on a Hercules card */
    SDL_Texture  *up;            /* integer-upscaled copy for sharp 4:3 scaling */
    int           up_scale, up_w;
    Uint32        ev_folder;     /* custom event: folder chosen in the dialog */
    char          chosen[1024];
    bool          dialog_open;
} App;

/* ------------------------------------------------------------------ drawing */

static void update_title(App *app)
{
    char t[256];
    if (!app->have_assets) {
        SDL_SetWindowTitle(app->win, APP_TITLE " - choose your game folder");
        return;
    }
    const Viewer *v = &app->viewer;
    if (app->playing) {
        SDL_snprintf(t, sizeof t, APP_TITLE " - %s - %s", app->game.title, viewer_mode_name(v));
        SDL_SetWindowTitle(app->win, t);
        return;
    }
    SDL_snprintf(t, sizeof t, APP_TITLE " - %s - %s%s", viewer_title(v, v->index), viewer_mode_name(v),
                 app->aspect_43 ? "" : " (square pixels)");
    SDL_SetWindowTitle(app->win, t);
}

/* Destination rectangle inside the window for a w x h screen, keeping the chosen aspect ratio. */
static SDL_FRect dest_rect(App *app, int w, int h, bool *integer)
{
    int ow, oh;
    SDL_GetRenderOutputSize(app->ren, &ow, &oh);
    SDL_FRect r;
    if (!app->aspect_43) {
        int k = SDL_min(ow / w, oh / h);
        if (k >= 1) {
            r.w = (float)(w * k);
            r.h = (float)(h * k);
            *integer = true;
        } else {
            float f = SDL_min((float)ow / w, (float)oh / h);
            r.w = w * f;
            r.h = h * f;
            *integer = false;
        }
    } else {
        r.h = (float)oh;
        r.w = r.h * 4.0f / 3.0f;
        if (r.w > ow) { r.w = (float)ow; r.h = r.w * 3.0f / 4.0f; }
        *integer = false;
    }
    r.x = SDL_floorf((ow - r.w) / 2);
    r.y = SDL_floorf((oh - r.h) / 2);
    return r;
}

static void render(App *app)
{
    SDL_SetRenderDrawColor(app->ren, 0, 0, 0, 255);
    SDL_RenderClear(app->ren);
    if (app->have_assets) {
        if (app->playing && app->battle) battle_draw(app->battle, &app->screen, app->viewer.mode);
        else if (app->playing) play_draw(&app->play, &app->screen, app->viewer.mode);
        else viewer_draw(&app->viewer, &app->screen);
        bool herc = app->viewer.herc && app->tex_herc;
        int w = herc ? HERC_W : SCREEN_W, h = herc ? HERC_H : SCREEN_H;
        SDL_Texture *tex = herc ? app->tex_herc : app->tex;
        if (herc) screen_to_herc(&app->screen, app->rgb);
        else screen_to_xrgb(&app->screen, app->rgb);
        SDL_UpdateTexture(tex, NULL, app->rgb, w * 4);
        bool integer;
        SDL_FRect dst = dest_rect(app, w, h, &integer);
        if (integer) {
            SDL_RenderTexture(app->ren, tex, NULL, &dst);
        } else {
            /* Sharp scaling: nearest-neighbour up to a whole multiple, then smooth down to fit. */
            int k = (int)SDL_ceilf(SDL_max(dst.w / w, dst.h / h));
            k = SDL_clamp(k, 1, 8);
            if (!app->up || app->up_scale != k || app->up_w != w) {
                SDL_DestroyTexture(app->up);
                app->up = SDL_CreateTexture(app->ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_TARGET,
                                            w * k, h * k);
                app->up_scale = k;
                app->up_w = w;
                if (app->up) SDL_SetTextureScaleMode(app->up, SDL_SCALEMODE_LINEAR);
            }
            if (app->up) {
                SDL_SetRenderTarget(app->ren, app->up);
                SDL_RenderTexture(app->ren, tex, NULL, NULL);
                SDL_SetRenderTarget(app->ren, NULL);
                SDL_RenderTexture(app->ren, app->up, NULL, &dst);
            } else {
                SDL_RenderTexture(app->ren, tex, NULL, &dst);
            }
        }
    }
    SDL_RenderPresent(app->ren);
}

/* ------------------------------------------------------------------ game folder */

static void pref_file(char *out, size_t n)
{
    char *pref = SDL_GetPrefPath(APP_ORG, APP_NAME);
    SDL_snprintf(out, n, "%sgamedir.txt", pref ? pref : "");
    SDL_free(pref);
}

static void save_game_dir(const char *dir)
{
    char path[1100];
    pref_file(path, sizeof path);
    SDL_IOStream *io = SDL_IOFromFile(path, "w");
    if (io) {
        SDL_WriteIO(io, dir, SDL_strlen(dir));
        SDL_CloseIO(io);
    }
}

static bool saved_game_dir(char *out, size_t n)
{
    char path[1100];
    size_t len = 0;
    pref_file(path, sizeof path);
    char *data = SDL_LoadFile(path, &len);
    if (!data) return false;
    while (len > 0 && (data[len - 1] == '\n' || data[len - 1] == '\r')) len--;
    data[len] = 0;
    SDL_strlcpy(out, data, n);
    SDL_free(data);
    return assets_dir_ok(out);
}

/* The window's icon (the Dock icon on macOS): the title page of the player's
 * own game files, as in the app bundle's icon (tools/make_icon.c). */
static void set_window_icon(App *app)
{
    enum { N = 256 };
    uint8_t *px = SDL_malloc(N * N * 4);
    if (px && app->win && icon_render(&app->ga, N, px)) {
        SDL_Surface *s = SDL_CreateSurfaceFrom(N, N, SDL_PIXELFORMAT_RGBA32, px, N * 4);
        if (s) { SDL_SetWindowIcon(app->win, s); SDL_DestroySurface(s); }
    }
    SDL_free(px);
}

static bool load_from(App *app, const char *dir, bool remember)
{
    char err[256] = "";
    if (app->have_assets) assets_free(&app->ga);
    app->have_assets = assets_load(&app->ga, dir, err, sizeof err);
    if (!app->have_assets) {
        SDL_Log("Could not load the game from %s: %s", dir, err);
        if (app->win)
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, APP_TITLE,
                                     "That folder doesn't hold a usable copy of The Ancient Art of War at Sea "
                                     "(WAS.EXE, SM, the SP files, AD, B and optionally WAST.DAT).", app->win);
        return false;
    }
    int n = campaigns_load(app->campaigns, dir);
    GfxMode mode = app->viewer.mode;
    bool herc = app->viewer.herc;
    viewer_init(&app->viewer, &app->ga, app->campaigns);
    if (app->ga.set[mode].loaded) app->viewer.mode = mode;
    app->viewer.herc = herc;
    if (remember) save_game_dir(dir);
    SDL_Log("Loaded game files from %s (%d campaigns; 16 colours: %s)", dir, n,
            app->ga.set[MODE_16].loaded ? "yes" : "no");
    set_window_icon(app);
    return true;
}

static void SDLCALL folder_chosen(void *userdata, const char *const *list, int filter)
{
    (void)filter;
    App *app = userdata;
    app->dialog_open = false;
    if (list && list[0]) {
        SDL_strlcpy(app->chosen, list[0], sizeof app->chosen);
        SDL_Event e;
        SDL_zero(e);
        e.type = app->ev_folder;
        SDL_PushEvent(&e);
    }
}

static void ask_for_folder(App *app)
{
    if (app->dialog_open) return;
    app->dialog_open = true;
    SDL_ShowOpenFolderDialog(folder_chosen, app, app->win, NULL, false);
}

/* Command line, then the remembered folder, then next to the app, then the current folder. */
static bool find_game_dir(const char *arg, char *out, size_t n)
{
    if (arg) { SDL_strlcpy(out, arg, n); return assets_dir_ok(out); }
    if (saved_game_dir(out, n)) return true;
    const char *base = SDL_GetBasePath();
    if (base) {
        SDL_snprintf(out, n, "%sgame", base);
        if (assets_dir_ok(out)) return true;
        SDL_strlcpy(out, base, n);
        if (assets_dir_ok(out)) return true;
    }
    SDL_strlcpy(out, ".", n);
    return assets_dir_ok(out);
}

/* ------------------------------------------------------------------ screenshots */

static int write_screenshots(App *app, const char *outdir)
{
    int written = 0;
    Viewer *v = &app->viewer;
    v->caption = false;
    for (int m = 0; m <= MODE_COUNT; m++) {  /* the last pass: Hercules */
        bool herc = m == MODE_COUNT;
        if (!herc && !app->ga.set[m].loaded) continue;
        v->mode = herc ? MODE_CGA : (GfxMode)m;
        v->herc = herc;
        int w = herc ? HERC_W : SCREEN_W, h = herc ? HERC_H : SCREEN_H;
        for (int i = 0; i < NUM_SCREENS; i++) {
            v->index = i;
            viewer_draw(v, &app->screen);
            if (herc) screen_to_herc(&app->screen, app->rgb);
            else screen_to_xrgb(&app->screen, app->rgb);
            SDL_Surface *surf = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_XRGB8888, app->rgb, w * 4);
            char path[1200];
            SDL_snprintf(path, sizeof path, "%s/%s_%02d.bmp", outdir, viewer_mode_name(v), i + 1);
            if (surf && SDL_SaveBMP(surf, path)) written++;
            else SDL_Log("Could not write %s: %s", path, SDL_GetError());
            SDL_DestroySurface(surf);
        }
    }
    SDL_Log("Wrote %d screenshots to %s", written, outdir);
    return written > 0 ? 0 : 1;
}

/* ------------------------------------------------------------------ playing */

/* SDL key to the original's scan code (play.h) */
static int play_keycode(SDL_Keycode k)
{
    switch (k) {
    case SDLK_E: return PK_E;   case SDLK_T: return PK_T;   case SDLK_Y: return PK_Y;
    case SDLK_I: return PK_I;   case SDLK_P: return PK_P;   case SDLK_S: return PK_S;
    case SDLK_D: return PK_D;   case SDLK_F: return PK_F;   case SDLK_J: return PK_J;
    case SDLK_Z: return PK_Z;   case SDLK_C: return PK_C;   case SDLK_N: return PK_N;
    case SDLK_M: return PK_M;
    case SDLK_RETURN: case SDLK_KP_ENTER: return PK_ENTER;
    case SDLK_SPACE: return PK_SPACE;
    case SDLK_MINUS: return PK_MINUS;
    case SDLK_EQUALS: case SDLK_PLUS: return PK_EQUALS;
    case SDLK_KP_MINUS: return PK_KP_MINUS;
    case SDLK_KP_PLUS: return PK_KP_PLUS;
    case SDLK_HOME: case SDLK_KP_7: return PK_HOME;
    case SDLK_UP: case SDLK_KP_8: return PK_UP;
    case SDLK_PAGEUP: case SDLK_KP_9: return PK_PGUP;
    case SDLK_LEFT: case SDLK_KP_4: return PK_LEFT;
    case SDLK_KP_5: return PK_KP_5;
    case SDLK_RIGHT: case SDLK_KP_6: return PK_RIGHT;
    case SDLK_END: case SDLK_KP_1: return PK_END;
    case SDLK_DOWN: case SDLK_KP_2: return PK_DOWN;
    case SDLK_PAGEDOWN: case SDLK_KP_3: return PK_PGDN;
    default: return PK_NONE;
    }
}

static bool is_cursor_pk(int pk) { return pk >= PK_HOME && pk <= PK_PGDN && pk != PK_KP_MINUS && pk != PK_KP_PLUS; }

/* The marker key held down: arrows combine into diagonals. In WAS.EXE a key
 * held down is read again every iteration (poll_input ovl4:8645). */
static int held_cursor_key(void)
{
    const bool *k = SDL_GetKeyboardState(NULL);
    if (k[SDL_SCANCODE_HOME] || k[SDL_SCANCODE_KP_7]) return PK_HOME;
    if (k[SDL_SCANCODE_PAGEUP] || k[SDL_SCANCODE_KP_9]) return PK_PGUP;
    if (k[SDL_SCANCODE_END] || k[SDL_SCANCODE_KP_1]) return PK_END;
    if (k[SDL_SCANCODE_PAGEDOWN] || k[SDL_SCANCODE_KP_3]) return PK_PGDN;
    int dx = (k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_KP_6]) - (k[SDL_SCANCODE_LEFT] || k[SDL_SCANCODE_KP_4]);
    int dy = (k[SDL_SCANCODE_DOWN] || k[SDL_SCANCODE_KP_2]) - (k[SDL_SCANCODE_UP] || k[SDL_SCANCODE_KP_8]);
    if (dx || dy) return PK_HOME + (dy + 1) * 4 + (dx + 1);
    if (k[SDL_SCANCODE_KP_MINUS]) return PK_KP_MINUS;   /* keypad -/+ repeat too */
    if (k[SDL_SCANCODE_KP_PLUS]) return PK_KP_PLUS;
    return PK_NONE;
}

static void start_play(App *app)
{
    const Campaign *c = &app->campaigns[app->viewer.index - SCREEN_MAP_FIRST];
    if (!c->loaded) return;
    app->game = *c;
    play_start(&app->play, &app->game, &app->ga, (unsigned)SDL_GetTicks());
    app->playing = true;
    app->tapped = 0;
}

/* One iteration of campaign play */
static void play_step(App *app)
{
    int held = held_cursor_key();
    if (!held && app->tapped) held = app->tapped;
    app->tapped = 0;
    play_frame(&app->play, held);
    if (app->play.result != PLAY_RUNNING) app->playing = false;   /* back to the viewer (the title page: step 6) */
    if (app->playing && app->play.battle_req) {                  /* Zoom or Practice: the sea battle */
        app->battle = SDL_malloc(sizeof *app->battle);
        if (!app->battle || !battle_start(app->battle, &app->play, app->play.battle_req == 2)) {
            SDL_free(app->battle);
            app->battle = NULL;
            app->play.battle_req = 0;
            return;
        }
        app->battle_ns = SDL_GetTicksNS();
    }
}

static void end_battle(App *app)
{
    battle_finish(app->battle);
    SDL_free(app->battle);
    app->battle = NULL;
}

/* The sea battle runs on its own clock; the screen is shown 60 times a second. */
static void battle_step(App *app)
{
    Uint64 now = SDL_GetTicksNS();
    battle_run(app->battle, (double)(now - app->battle_ns) / 1000.0);
    app->battle_ns = now;
    if (app->battle->state != BATTLE_RUNNING) end_battle(app);
}

/* An SDL key as WAS.EXE's battle reads it (DOS function 08h): a character, or 0
 * and the scan code for the cursor keys and the keypad (NumLock is off) */
static void battle_keydown(App *app, SDL_Keycode k, SDL_Keymod mod)
{
    bool shift = (mod & SDL_KMOD_SHIFT) != 0;
    int scan = 0, ch = 0;
    switch (k) {
    case SDLK_HOME: case SDLK_KP_7: scan = 0x47; break;
    case SDLK_UP: case SDLK_KP_8: scan = 0x48; break;
    case SDLK_PAGEUP: case SDLK_KP_9: scan = 0x49; break;
    case SDLK_LEFT: case SDLK_KP_4: scan = 0x4B; break;
    case SDLK_RIGHT: case SDLK_KP_6: scan = 0x4D; break;
    case SDLK_END: case SDLK_KP_1: scan = 0x4F; break;
    case SDLK_DOWN: case SDLK_KP_2: scan = 0x50; break;
    case SDLK_PAGEDOWN: case SDLK_KP_3: scan = 0x51; break;
    case SDLK_RETURN: case SDLK_KP_ENTER: ch = 13; break;
    case SDLK_ESCAPE: ch = 27; break;
    case SDLK_SPACE: ch = ' '; break;
    case SDLK_BACKSPACE: ch = 8; break;
    case SDLK_KP_PLUS: ch = '+'; break;
    case SDLK_KP_MINUS: ch = '-'; break;
    case SDLK_MINUS: ch = shift ? '_' : '-'; break;
    case SDLK_EQUALS: ch = shift ? '+' : '='; break;
    case SDLK_PLUS: ch = '+'; break;
    default:
        if (k >= SDLK_A && k <= SDLK_Z) ch = (int)(k - SDLK_A) + (shift ? 'A' : 'a');
        else if (k >= SDLK_0 && k <= SDLK_9) ch = (int)(k - SDLK_0) + '0';
        break;
    }
    if (scan) battle_key(app->battle, 0, scan);
    else if (ch) battle_key(app->battle, ch, 0);
}

/* ------------------------------------------------------------------ main */

static void toggle_fullscreen(App *app)
{
    bool full = (SDL_GetWindowFlags(app->win) & SDL_WINDOW_FULLSCREEN) != 0;
    SDL_SetWindowFullscreen(app->win, !full);
}

static ViewerKey viewer_keycode(SDL_Keycode k, bool shift)
{
    bool up = k == SDLK_UP || k == SDLK_MINUS || k == SDLK_KP_MINUS;
    bool down = k == SDLK_DOWN || k == SDLK_EQUALS || k == SDLK_PLUS || k == SDLK_KP_PLUS;
    if (up)   return shift ? VK_TOP : VK_UP;
    if (down) return shift ? VK_BOTTOM : VK_DOWN;
    switch (k) {
    case SDLK_RIGHT: case SDLK_SPACE: case SDLK_PAGEDOWN: return VK_NEXT;
    case SDLK_LEFT: case SDLK_BACKSPACE: case SDLK_PAGEUP: return VK_PREV;
    case SDLK_HOME:   return VK_TOP;
    case SDLK_END:    return VK_BOTTOM;
    case SDLK_TAB:    return VK_MODE;
    case SDLK_H:      return VK_CAPTION;
    case SDLK_I:      return VK_STORY;
    default:          return VK_NONE;
    }
}

int main(int argc, char **argv)
{
    static App app;   /* large: keep off the stack */
    const char *dir_arg = NULL, *shots = NULL;
    for (int i = 1; i < argc; i++) {
        if (SDL_strcmp(argv[i], "--screenshots") == 0 && i + 1 < argc) shots = argv[++i];
        else if (SDL_strcasecmp(argv[i], "HERC") == 0 || SDL_strcasecmp(argv[i], "--herc") == 0) app.viewer.herc = true;
        else if (argv[i][0] != '-') dir_arg = argv[i];
    }
    app.aspect_43 = true;

    char dir[1024];
    bool found = find_game_dir(dir_arg, dir, sizeof dir);

    if (shots) {
        if (!found || !load_from(&app, dir, false)) {
            SDL_Log("Game files not found. Usage: --screenshots OUTDIR GAME-FOLDER");
            return 1;
        }
        int rc = write_screenshots(&app, shots);
        assets_free(&app.ga);
        return rc;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    if (!SDL_CreateWindowAndRenderer(APP_TITLE, 1280, 960,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, &app.win, &app.ren)) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(app.ren, 1);
    app.tex = SDL_CreateTexture(app.ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);
    SDL_SetTextureScaleMode(app.tex, SDL_SCALEMODE_NEAREST);
    app.tex_herc = SDL_CreateTexture(app.ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, HERC_W, HERC_H);
    if (app.tex_herc) SDL_SetTextureScaleMode(app.tex_herc, SDL_SCALEMODE_NEAREST);
    app.ev_folder = SDL_RegisterEvents(1);

    if (found) load_from(&app, dir, dir_arg != NULL);
    if (!app.have_assets) ask_for_folder(&app);
    update_title(&app);

    bool running = true;
    Uint64 next_frame = SDL_GetTicksNS();
    const Uint64 frame_ns = SDL_NS_PER_SECOND / PLAY_HZ;
    while (running) {
        SDL_Event e;
        if (app.playing && app.battle) {
            Uint64 now = SDL_GetTicksNS();
            if (now >= next_frame) {
                next_frame = now + SDL_NS_PER_SECOND / 60;
                battle_step(&app);
                render(&app);
                continue;
            }
            if (!SDL_WaitEventTimeout(&e, (Sint32)((next_frame - now) / SDL_NS_PER_MS) + 1)) continue;
        } else if (app.playing) {
            Uint64 now = SDL_GetTicksNS();
            if (now >= next_frame) {
                next_frame = (now - next_frame > frame_ns * 4) ? now + frame_ns : next_frame + frame_ns;
                play_step(&app);
                if (!app.playing) update_title(&app);
                render(&app);
                continue;
            }
            if (!SDL_WaitEventTimeout(&e, (Sint32)((next_frame - now) / SDL_NS_PER_MS) + 1)) continue;
        } else {
            next_frame = SDL_GetTicksNS();
            if (!SDL_WaitEventTimeout(&e, 500)) { render(&app); continue; }
        }
        bool dirty = false;
        do {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (e.type == app.ev_folder) {
                if (!load_from(&app, app.chosen, true)) ask_for_folder(&app);
                dirty = true;
            } else if (e.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode k = e.key.key;
                bool mod = (e.key.mod & (SDL_KMOD_ALT | SDL_KMOD_GUI)) != 0;
                if (k == SDLK_Q && (e.key.mod & SDL_KMOD_GUI)) running = false;
                else if (k == SDLK_RETURN && mod) toggle_fullscreen(&app);
                else if (app.playing && app.battle) {
                    if (k == SDLK_TAB) viewer_key(&app.viewer, VK_MODE);
                    else if (k == SDLK_F2) app.aspect_43 = !app.aspect_43;
                    else battle_keydown(&app, k, e.key.mod);
                }
                else if (app.playing) {
                    if (k == SDLK_TAB) viewer_key(&app.viewer, VK_MODE);
                    else if (k == SDLK_F2) app.aspect_43 = !app.aspect_43;
                    else {
                        if (e.key.repeat) continue;
                        int pk = k == SDLK_ESCAPE ? PK_ESC : play_keycode(k);
                        if (pk == PK_S && (e.key.mod & SDL_KMOD_ALT)) pk = PK_ALT_S;
                        if (is_cursor_pk(pk)) app.tapped = pk;
                        else if (pk) play_key(&app.play, pk, (e.key.mod & SDL_KMOD_SHIFT) != 0);
                    }
                }
                else if (k == SDLK_ESCAPE) running = false;
                else if ((k == SDLK_RETURN || k == SDLK_KP_ENTER) && app.have_assets && viewer_is_map(&app.viewer) &&
                         app.viewer.page == 0) start_play(&app);
                else if (k == SDLK_F) toggle_fullscreen(&app);
                else if (k == SDLK_O) ask_for_folder(&app);
                else if (k == SDLK_F2 || k == SDLK_A) app.aspect_43 = !app.aspect_43;
                else if (app.have_assets) viewer_key(&app.viewer, viewer_keycode(k, (e.key.mod & SDL_KMOD_SHIFT) != 0));
                dirty = true;
            } else if (e.type == SDL_EVENT_MOUSE_WHEEL && app.have_assets && !app.playing) {
                viewer_scroll(&app.viewer, (int)SDL_roundf(-e.wheel.y * TILE_H / 2));
                dirty = true;
            } else if (e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || e.type == SDL_EVENT_WINDOW_EXPOSED) {
                dirty = true;
            }
        } while (SDL_PollEvent(&e));
        if (dirty) {
            update_title(&app);
            render(&app);
        }
    }

    if (app.battle) { fiber_stop(&app.battle->fiber); SDL_free(app.battle); }
    if (app.have_assets) assets_free(&app.ga);
    SDL_DestroyTexture(app.up);
    SDL_DestroyTexture(app.tex);
    SDL_DestroyTexture(app.tex_herc);
    SDL_DestroyRenderer(app.ren);
    SDL_DestroyWindow(app.win);
    SDL_Quit();
    return 0;
}
