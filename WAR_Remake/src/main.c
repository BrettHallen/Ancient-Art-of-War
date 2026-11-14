/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * main.c: the title pages and menus (step 6), campaign play (step 3), battles
 * (step 4), and the asset viewer (step 1).
 *
 * The game starts as the original does: "presented by", the title page, the
 * scrolls (titles, story, rules), the opponents and the quote. Everything is
 * drawn from the player's own copy of the original game, in CGA or VGA colours,
 * or as WAR.EXE showed it on a Hercules monochrome card (WAR HERC).
 *
 * Menus: -/+ (or the arrows) change, Enter selects, Esc backs up, Space
 *        changes a rule or an option. F12 opens the asset viewer.
 *        Left alone on the title page for 30 seconds, the game plays a demo
 *        until a key is pressed (as the original does).
 *
 * Viewer keys:  Left/Right  previous/next screen     Tab   CGA / EGA / Tandy / VGA / Hercules
 *        Up/Down, -/+, mouse wheel   scroll a campaign map
 *        Shift+-/+, Home/End         top / bottom of the map
 *        Enter       play the campaign shown
 *        A           4:3 or square pixels     H     show/hide caption
 *        F, Alt+Enter, Cmd+Enter  full screen Esc   back to the title page
 *
 * Playing: the original keys (arrows or keypad move the cursor, M Move,
 *        I Info, S Size/Surrender/Speed, T Time, C Clear/Cancel, V View,
 *        E End, Z Zoom, -/+ scroll, Option+S on a Mac or Alt+S save (SDL's Alt is the Option
 *        key), Enter at the end), plus Tab the video card,
 *        F2 aspect, Alt/Cmd+Enter full screen, Esc leave the game.
 * Battle (Zoom): A Attack, R Retreat; then A Archers, B Barbarians, K Knights,
 *        S Squad and A Attack, F Forward, B Backoff, S Stop, Backspace back.
 *
 * Usage: AncientArtOfWar [video=CARD] [--viewer] [--log | --log=verbose] [game-folder]
 *          video=: the card to start on, CGA if not given: cga, composite (a CGA on a TV), composite2
 *          (the new CGA on a TV), ega, tandy, vga, herc; HERC is video=herc, as WAR HERC
 *          --log: a debug log of each campaign played (gamelog.h), in the
 *          remake's folder under Logs/; --log=verbose (or --log verbose) for more
 *        AncientArtOfWar --screenshots OUTDIR [game-folder]   (write BMPs, no window)
 */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "assets.h"
#include "campaign.h"
#include "gfx.h"
#include "play.h"
#include "battle.h"
#include "menu.h"
#include "sound.h"
#include "icon.h"
#include "gamelog.h"

#define APP_ORG  "AncientArtOfWar"
#define APP_NAME "Remake"

enum { SCREEN_TILES = PIC_COUNT, SCREEN_FONT, SCREEN_MAP_FIRST, NUM_SCREENS = SCREEN_MAP_FIRST + NUM_CAMPAIGNS };

#define SCROLL_BOTTOM (MAP_PIXEL_H - MAP_VIEW_H)

typedef struct {
    GameAssets    ga;
    bool          have_assets;
    Campaign      campaigns[NUM_CAMPAIGNS];
    int           scroll[NUM_CAMPAIGNS];   /* pixels from the top of each map */
    GfxMode       mode;          /* the pictures shown: CGA, or the 16-colour set (EGA, Tandy, VGA) */
    bool          herc;          /* shown as on a Hercules card (drawn in CGA) */
    VideoCard     card;          /* the card: mode and herc follow it (set_card) */
    int           index;
    bool          aspect_43;     /* true: 4:3 like a CRT, false: square pixels */
    bool          caption;
    Screen        screen;
    uint32_t      rgb[HERC_W * HERC_H];   /* the screen: 320x200, or 720x348 on a Hercules card */
    SDL_Window   *win;
    SDL_Renderer *ren;
    SDL_Texture  *tex;           /* 320x200 game screen */
    SDL_Texture  *tex_herc;      /* 720x348 on a Hercules card */
    SDL_Texture  *tex_comp;      /* 640x200 for a CGA on a TV (composite) */
    SDL_Texture  *up;            /* integer-upscaled copy for sharp 4:3 scaling */
    int           up_scale, up_w;
    bool          playing;
    bool          in_menu;       /* the title pages and menus */
    Menu         *menu;
    Campaign      game;          /* the campaign being played (with its chosen rules and opponent) */
    Play          play;
    bool          in_battle;     /* Zoom: the battle screen */
    int           demo_extra;    /* the demo: extra scrolling turns owed, in halves */
    Battle        battle;
    int           tapped;        /* cursor key pressed since the last frame */
    Uint32        ev_folder;     /* custom event: folder chosen in the dialog */
    char          chosen[1024];
    bool          dialog_open;
} App;

/* ------------------------------------------------------------------ drawing */

static const char *screen_title(const App *app, int index)
{
    if (index == SCREEN_TILES) return "Map tiles";
    if (index == SCREEN_FONT)  return "Font";
    if (index >= SCREEN_MAP_FIRST) return app->campaigns[index - SCREEN_MAP_FIRST].title;
    return picture_name((PictureId)index);
}

static bool is_map(const App *app) { return app->index >= SCREEN_MAP_FIRST; }

static void scroll_map(App *app, int delta)
{
    if (!is_map(app)) return;
    int *sc = &app->scroll[app->index - SCREEN_MAP_FIRST];
    *sc = SDL_clamp(*sc + delta, 0, SCROLL_BOTTOM);
}

/* Options Shoppe: "display is black/white" (WAR.EXE's; WARET.EXE has no such line) */
static void apply_display(App *app, Screen *s)
{
    if (!app->menu || menu_options(app->menu)->display != 0 || app->mode != MODE_CGA) return;
    for (int i = 0; i < 16; i++) {
        int g = (s->pal[i].r * 30 + s->pal[i].g * 59 + s->pal[i].b * 11) / 100;
        s->pal[i] = (RGB){ (uint8_t)g, (uint8_t)g, (uint8_t)g };
    }
}

/* the card shown (mode and herc decide; card tells EGA, Tandy and VGA apart) */
static VideoCard current_card(const App *app)
{
    if (app->herc) return CARD_HERC;
    if (app->mode == MODE_CGA) return app->card == CARD_COMPOSITE || app->card == CARD_COMPOSITE2 ? app->card : CARD_CGA;
    return app->card == CARD_EGA || app->card == CARD_TANDY ? app->card : CARD_VGA;
}

static void set_card(App *app, VideoCard c)
{
    if (card_assets(c) == MODE_VGA && !app->ga.set[MODE_VGA].loaded) c = CARD_CGA;   /* no WARET.DAT */
    app->card = c;
    app->mode = card_assets(c);
    app->herc = c == CARD_HERC;
}

/* The Options Shoppe's display line and the CGA cards go together (a remake convenience: in WAR.EXE
 * the setting only set a PCjr's or Tandy's palette registers, and a plain CGA looked the same):
 * tv/composite is the CGA on a TV (CARD_COMPOSITE, or the new CGA's if that is shown), rgb the plain
 * CGA; black/white keeps the card and Tab keeps black/white. card_to_display: after Tab or at the start, the line follows the
 * card; display_to_card: after the line was changed in the Options Shoppe, the card follows it. */
static void card_to_display(App *app)
{
    if (!app->menu || app->herc || app->mode != MODE_CGA) return;
    int d = menu_options(app->menu)->display;
    VideoCard c = current_card(app);
    if (c == CARD_COMPOSITE || c == CARD_COMPOSITE2) { if (d == 2) menu_set_display(app->menu, 1); }   /* black/white stays: a TV in greys */
    else if (d == 1) menu_set_display(app->menu, 2);
}

static void display_to_card(App *app, int before)
{
    if (!app->menu || app->herc || app->mode != MODE_CGA) return;
    int d = menu_options(app->menu)->display;
    if (d == before) return;
    VideoCard c = current_card(app);
    if (d == 1 && c != CARD_COMPOSITE && c != CARD_COMPOSITE2) set_card(app, CARD_COMPOSITE);
    else if (d == 2 && c != CARD_CGA) set_card(app, CARD_CGA);
}

/* "CGA", "EGA", "Tandy", "VGA" or "Hercules" */
static const char *display_name(const App *app) { return card_name(current_card(app)); }

/* Tab: CGA, CGA composite (old, new), EGA, Tandy, VGA (the three if WARET.DAT is there), Hercules, then CGA again */
static void next_mode(App *app)
{
    VideoCard c = current_card(app);
    do c = (VideoCard)((c + 1) % CARD_COUNT);
    while (card_assets(c) == MODE_VGA && !app->ga.set[MODE_VGA].loaded);
    set_card(app, c);
    card_to_display(app);
}

static bool composite(const App *app) { VideoCard c = current_card(app); return c == CARD_COMPOSITE || c == CARD_COMPOSITE2; }

/* the screen's size in pixels for the current card */
static int out_w(const App *app) { return app->herc ? HERC_W : composite(app) ? COMP_W : SCREEN_W; }
static int out_h(const App *app) { return app->herc ? HERC_H : SCREEN_H; }

static void to_pixels(App *app, const Screen *s)
{
    if (app->herc) screen_to_herc(s, app->rgb);
    else if (composite(app)) screen_to_composite(s, current_card(app) == CARD_COMPOSITE2, app->rgb);
    else screen_to_xrgb(s, app->rgb);
}

static void draw_screen(App *app)
{
    Screen *s = &app->screen;
    GfxMode mode = app->herc ? MODE_CGA : app->mode;   /* a Hercules card shows the CGA screen */
    gfx_card = current_card(app);
    const AssetSet *set = &app->ga.set[mode];
    int ncolors = mode == MODE_VGA ? 16 : 4;

    if (app->in_menu && app->menu) {
        menu_draw(app->menu, s, mode);
        apply_display(app, s);
        to_pixels(app, s);
        return;
    }
    if (app->playing && app->in_battle) {
        battle_draw(&app->battle, s, mode);
        apply_display(app, s);
        to_pixels(app, s);
        return;
    }
    if (app->playing) {
        play_draw(&app->play, s, mode);
        apply_display(app, s);
        to_pixels(app, s);
        return;
    }
    if (app->index < PIC_COUNT) {
        const Image *img = &set->pictures[app->index];
        screen_set_palette(s, img->pal, img->ncolors);
        screen_blit(s, img, 0, 0);
    } else if (app->index >= SCREEN_MAP_FIRST) {
        const Campaign *c = &app->campaigns[app->index - SCREEN_MAP_FIRST];
        campaign_draw_map(s, &app->ga, mode, c, app->scroll[app->index - SCREEN_MAP_FIRST]);
        uint8_t white = screen_nearest(s, ncolors, 255, 255, 255);
        uint8_t black = screen_nearest(s, ncolors, 0, 0, 0);
        screen_fill(s, 0, MAP_VIEW_H, SCREEN_W, SCREEN_H - MAP_VIEW_H, black);
        char line[64];
        if (c->loaded) SDL_snprintf(line, sizeof line, "%s", c->title);
        else           SDL_snprintf(line, sizeof line, "%s not found", c->file);
        line[40] = 0;
        screen_text(s, &app->ga, 0, MAP_VIEW_H + 1, line, white, black);
        if (c->loaded) {
            SDL_snprintf(line, sizeof line, "vs %s", OPPONENT_NAMES[c->opponent]);
            screen_text(s, &app->ga, 0, MAP_VIEW_H + 11, line, white, black);
            SDL_snprintf(line, sizeof line, "%2d v %2d", c->num_friendly, c->num_enemy);
            screen_text(s, &app->ga, SCREEN_W - 7 * 8, MAP_VIEW_H + 11, line, white, black);
        }
    } else if (app->index == SCREEN_TILES) {
        screen_set_palette(s, set->tiles[0].pal, set->tiles[0].ncolors);
        screen_clear(s, 0);
        for (int t = 0; t < NUM_TILES; t++)
            screen_blit(s, &set->tiles[t], (t % 10) * TILE_W, (t / 10) * TILE_H);
    } else {
        if (mode == MODE_CGA) {
            screen_set_palette(s, CGA_PALETTE, 4);
        } else {
            RGB pal[16] = { { 0, 0, 0 } };
            pal[15] = (RGB){ 0xFF, 0xFF, 0xFF };
            screen_set_palette(s, pal, 16);
        }
        screen_clear(s, 0);
        uint8_t white = screen_nearest(s, ncolors, 255, 255, 255);
        char line[17];
        for (int row = 0; row < 6; row++) {
            int n = 0;
            for (int c = 0; c < 16; c++) {
                int ch = FONT_FIRST + row * 16 + c;
                line[n++] = (ch < FONT_FIRST + FONT_COUNT) ? (char)ch : ' ';
            }
            line[n] = 0;
            screen_text(s, &app->ga, 32, 40 + row * 16, line, white, -1);
        }
        screen_text(s, &app->ga, 32, 16, "The Ancient Art of War font", white, -1);
    }

    if (app->caption && app->index < SCREEN_MAP_FIRST) {   /* maps use the bottom lines themselves */
        uint8_t white = screen_nearest(s, ncolors, 255, 255, 255);
        uint8_t black = screen_nearest(s, ncolors, 0, 0, 0);
        char cap[64];
        SDL_snprintf(cap, sizeof cap, "%s %d/%d: %s", display_name(app), app->index + 1, NUM_SCREENS,
                     screen_title(app, app->index));
        cap[40] = 0;   /* 40 characters per line */
        screen_fill(s, 0, SCREEN_H - 8, SCREEN_W, 8, black);
        screen_text(s, &app->ga, 0, SCREEN_H - 8, cap, white, black);
    }
    to_pixels(app, s);
}

static void update_title(App *app)
{
    char t[256];
    if (!app->have_assets) {
        SDL_SetWindowTitle(app->win, "The Ancient Art of War - choose your game folder");
        return;
    }
    if (app->playing) {
        SDL_snprintf(t, sizeof t, "The Ancient Art of War - %s - %s", app->play.c->title, display_name(app));
        SDL_SetWindowTitle(app->win, t);
        return;
    }
    SDL_snprintf(t, sizeof t, "The Ancient Art of War - %s - %s%s", screen_title(app, app->index), display_name(app),
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
        if (app->herc && !app->tex_herc) app->herc = false;
        if (composite(app) && !app->tex_comp) set_card(app, CARD_CGA);
        draw_screen(app);
        int w = out_w(app), h = out_h(app);
        SDL_Texture *tex = app->herc ? app->tex_herc : composite(app) ? app->tex_comp : app->tex;
        SDL_UpdateTexture(tex, NULL, app->rgb, w * 4);
        bool integer;
        /* a composite screen is 640 hdots for 320 pixels: placed as 320 x 200, scaled sharply */
        SDL_FRect dst = dest_rect(app, w == COMP_W ? SCREEN_W : w, h, &integer);
        if (w == COMP_W) integer = false;
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

/* The window's icon (the Dock icon on macOS): the opening title page of the
 * player's own game files, as in the app bundle's icon (tools/make_icon.c). */
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
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "The Ancient Art of War",
                                     "That folder doesn't hold a usable copy of The Ancient Art of War "
                                     "(WAR.EXE, M, the P files and optionally WARET.DAT).", app->win);
        return false;
    }
    set_card(app, current_card(app));   /* without WARET.DAT: CGA */
    int n = campaigns_load(app->campaigns, dir);
    for (int i = 0; i < NUM_CAMPAIGNS; i++) app->scroll[i] = SCROLL_BOTTOM;   /* start at the bottom, like the game */
    SDL_Log("Loaded %d campaigns", n);
    if (remember) save_game_dir(dir);
    SDL_Log("Loaded game files from %s (VGA graphics: %s)", dir, app->ga.set[MODE_VGA].loaded ? "yes" : "no");
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
    for (int c = 0; c < CARD_COUNT; c++) {   /* CGA, EGA, Tandy, VGA, Hercules */
        if (card_assets((VideoCard)c) == MODE_VGA && !app->ga.set[MODE_VGA].loaded) continue;
        set_card(app, (VideoCard)c);
        int w = out_w(app), h = out_h(app);
        for (int i = 0; i < NUM_SCREENS; i++) {
            app->index = i;
            draw_screen(app);
            SDL_Surface *surf = SDL_CreateSurfaceFrom(w, h, SDL_PIXELFORMAT_XRGB8888, app->rgb, w * 4);
            char path[1200];
            SDL_snprintf(path, sizeof path, "%s/%s_%02d.bmp", outdir, display_name(app), i + 1);
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
    case SDLK_I: return PK_I;   case SDLK_S: return PK_S;   case SDLK_D: return PK_D;
    case SDLK_F: return PK_F;   case SDLK_J: return PK_J;   case SDLK_Z: return PK_Z;
    case SDLK_C: return PK_C;   case SDLK_V: return PK_V;   case SDLK_N: return PK_N;
    case SDLK_M: return PK_M;   case SDLK_A: return PK_A;   case SDLK_B: return PK_B;
    case SDLK_K: return PK_K;   case SDLK_R: return PK_R;   case SDLK_BACKSPACE: return PK_BACKSPACE;
    case SDLK_Q: return PK_Q;   case SDLK_L: return PK_L;
    case SDLK_W: return PK_W;   case SDLK_U: return PK_U;   case SDLK_O: return PK_O;
    case SDLK_P: return PK_P;   case SDLK_G: return PK_G;   case SDLK_H: return PK_H;
    case SDLK_X: return PK_X;
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

/* The cursor key held down: arrows combine into diagonals. */
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

static void begin_game(App *app, const Campaign *c, uint32_t seed)
{
    app->game = *c;
    int fight_delay = app->menu ? menu_options(app->menu)->fight_delay : 400;
    app->play.c = &app->game;          /* the log's header comes from the campaign */
    app->play.ga = &app->ga;
    app->play.demo = 0;
    gamelog_begin(&app->play, &app->game, "New campaign", seed, fight_delay);
    play_start(&app->play, &app->game, &app->ga, seed);
    if (app->menu) app->play.fight_delay = fight_delay;   /* [A6E3] */
    app->playing = true;
    app->in_menu = false;
    app->in_battle = false;
    app->tapped = 0;
}

static void begin_demo(App *app, const Campaign *c, uint32_t seed)
{
    gamelog_end(&app->play, "Left for the demo");   /* the demo isn't logged */
    app->game = *c;
    play_start_demo(&app->play, &app->game, &app->ga, seed);
    if (app->menu) app->play.fight_delay = menu_options(app->menu)->fight_delay;
    app->playing = true;
    app->in_menu = false;
    app->in_battle = false;
    app->tapped = 0;
}

/* The demo is over (a key, a battle, the end of its game): the title page,
 * as game_init shows it again (main rt:00BC, battle_report ovl1:53B4). */
static void end_demo(App *app)
{
    app->playing = false;
    app->in_battle = false;
    app->in_menu = app->menu != NULL;
    if (app->menu) menu_title(app->menu);
}

static void start_play(App *app)
{
    const Campaign *c = &app->campaigns[app->index - SCREEN_MAP_FIRST];
    if (!c->loaded) return;
    begin_game(app, c, (uint32_t)(SDL_GetTicks() / 55));   /* like the BIOS tick count */
}

static void to_menu(App *app)
{
    app->playing = false;
    app->in_battle = false;
    app->in_menu = app->menu != NULL;
}

/* What the menus asked for */
static bool menu_step(App *app)
{
    if (menu_wants_held(app->menu)) {  /* the squads editor: the cursor moves while a key is held */
        int held = held_cursor_key();
        if (!held && app->tapped) held = app->tapped;
        menu_held(app->menu, held);
    }
    app->tapped = 0;
    menu_frame(app->menu);
    switch (menu_action(app->menu)) {
    case MENU_START_GAME:
        begin_game(app, menu_game(app->menu), menu_seed(app->menu));
        break;
    case MENU_RESUME_GAME:
        if (menu_load_saved_game(app->menu, &app->play, &app->game)) {
            app->play.fight_delay = menu_options(app->menu)->fight_delay;
            gamelog_begin(&app->play, &app->game, "Saved game resumed (S on the data disk)", app->play.rnd, app->play.fight_delay);
            gamelog_state(&app->play, "The saved game");
            play_resume(&app->play);
            app->playing = true;
            app->in_menu = false;
            app->in_battle = false;
        } else {
            menu_saved_game_failed(app->menu);
        }
        break;
    case MENU_CONTINUE:
        play_resume(&app->play);
        app->playing = true;
        app->in_menu = false;
        break;
    case MENU_DEMO:
        begin_demo(app, menu_game(app->menu), menu_seed(app->menu));
        break;
    case MENU_QUIT:
        return false;
    default:
        break;
    }
    return true;
}

static bool demo_scrolling(const Play *p)
{
    return p->demo == 1 && p->result == PLAY_RUNNING && p->zoom_squad < 0 &&
           (p->demo_key == PK_KP_MINUS || p->demo_key == PK_KP_PLUS);
}

/* One iteration of the campaign; false when it has ended. */
static bool play_step(App *app)
{
    if (app->in_battle) {
        app->tapped = 0;
        battle_frame(&app->battle);
        if (app->battle.done) {
            battle_finish(&app->battle);
            app->in_battle = false;
            if (app->play.demo) { end_demo(app); return false; }   /* rt:00BC: back to game_init */
        }
        return true;
    }
    if (app->play.zoom_squad >= 0) {   /* Zoom: campaign_play returns 2, main calls battle */
        if (battle_start(&app->battle, &app->play, app->play.zoom_squad)) {
            app->in_battle = true;
            return true;
        }
        app->play.zoom_squad = -1;     /* no O6/O7/F files */
        if (app->play.demo) { end_demo(app); return false; }
    }
    int held = held_cursor_key();
    if (!held && app->tapped) held = app->tapped;
    app->tapped = 0;
    play_frame(&app->play, held);
    /* The demo's scrolling turns of the loop skip the game's work, and ran about
     * 50 a second on the PCs of the day (DOSBox at 1500 cycles): 5 per 2 frames. */
    if (demo_scrolling(&app->play)) {
        app->demo_extra += 3;
        while (app->demo_extra >= 2 && demo_scrolling(&app->play)) {
            app->demo_extra -= 2;
            play_frame(&app->play, 0);
        }
    } else {
        app->demo_extra = 0;
    }
    if (app->play.result != PLAY_RUNNING) {
        PlayResult r = app->play.result;
        if (r == PLAY_SAVE) {
            GLOG(&app->play, GLOG_NORMAL, "The game is saved; the log stays open in case play goes on");
        } else {
            static const struct { int code; const char *s; } WHY[] = {
                { 0, "You won: the enemy has no flags" }, { 1, "You won: the enemy surrendered" },
                { 2, "You won: the enemy has no squads" }, { 10, "You lost: you have no flags" },
                { 11, "You lost: you surrendered" }, { 12, "You lost: you have no squads" } };
            const char *why = "You quit the game";
            if (r == PLAY_WON || r == PLAY_LOST)
                for (size_t k = 0; k < SDL_arraysize(WHY); k++) if (WHY[k].code == app->play.result_code) why = WHY[k].s;
            gamelog_end(&app->play, why);
        }
        to_menu(app);
        if (app->menu) {
            if (r == PLAY_SAVE) menu_save(app->menu, &app->play);               /* thunk 4 */
            else if (r == PLAY_WON || r == PLAY_LOST) menu_report(app->menu, &app->play);   /* thunk 5 */
            else menu_title(app->menu);
        }
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------ main */

static void toggle_fullscreen(App *app)
{
    bool full = (SDL_GetWindowFlags(app->win) & SDL_WINDOW_FULLSCREEN) != 0;
    SDL_SetWindowFullscreen(app->win, !full);
}

int main(int argc, char **argv)
{
    static App app;   /* large: keep off the stack */
    const char *dir_arg = NULL, *shots = NULL;
    bool viewer = false;
    GameLogLevel log_level = GLOG_OFF;
    VideoCard card = CARD_CGA;          /* video=: the card to start on */
    for (int i = 1; i < argc; i++) {
        if (SDL_strcmp(argv[i], "--screenshots") == 0 && i + 1 < argc) shots = argv[++i];
        else if (SDL_strcmp(argv[i], "--viewer") == 0) viewer = true;
        else if (SDL_strcasecmp(argv[i], "HERC") == 0 || SDL_strcmp(argv[i], "--herc") == 0) card = CARD_HERC;   /* WAR HERC (rt:000C) */
        else if (SDL_strncasecmp(argv[i], "video=", 6) == 0 || SDL_strncasecmp(argv[i], "--video=", 8) == 0) {
            const char *v = SDL_strchr(argv[i], '=') + 1;
            if (!card_from_name(v, &card)) SDL_Log("video=%s: use cga, composite, composite2, ega, tandy, vga or herc", v);
        }
        else if (SDL_strcmp(argv[i], "--video") == 0 && i + 1 < argc) {
            if (!card_from_name(argv[++i], &card)) SDL_Log("--video %s: use cga, composite, composite2, ega, tandy, vga or herc", argv[i]);
        }
        else if (SDL_strcmp(argv[i], "--log=verbose") == 0) log_level = GLOG_VERBOSE;
        else if (SDL_strcmp(argv[i], "--log") == 0 || SDL_strcmp(argv[i], "--log=normal") == 0) {
            log_level = GLOG_NORMAL;
            if (i + 1 < argc && SDL_strcmp(argv[i + 1], "verbose") == 0) { log_level = GLOG_VERBOSE; i++; }
            else if (i + 1 < argc && SDL_strcmp(argv[i + 1], "normal") == 0) i++;
        }
        else if (argv[i][0] != '-') dir_arg = argv[i];
    }
    app.aspect_43 = true;
    app.caption = true;
    app.card = card;
    app.mode = card_assets(card);
    app.herc = card == CARD_HERC;

    char dir[1024];
    bool found = find_game_dir(dir_arg, dir, sizeof dir);

    if (shots) {
        app.caption = false;
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
    if (!SDL_CreateWindowAndRenderer("The Ancient Art of War", 1280, 960,
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
    app.tex_comp = SDL_CreateTexture(app.ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, COMP_W, SCREEN_H);
    if (app.tex_comp) SDL_SetTextureScaleMode(app.tex_comp, SDL_SCALEMODE_NEAREST);
    app.ev_folder = SDL_RegisterEvents(1);
    sound_init();                       /* without an audio device the game runs silent */
    if (log_level != GLOG_OFF) {
        char *pref = SDL_GetPrefPath(APP_ORG, APP_NAME);
        char logs[1100];
        SDL_snprintf(logs, sizeof logs, "%sLogs/", pref ? pref : "");
        SDL_free(pref);
        gamelog_setup(log_level, logs);
        SDL_Log("Game logs (%s) go to %s", log_level == GLOG_VERBOSE ? "verbose" : "normal", logs);
    }

    if (found) load_from(&app, dir, dir_arg == NULL ? false : true);
    if (!app.have_assets) ask_for_folder(&app);
    else if (!viewer && app.ga.have_menus) {
        char *pref = SDL_GetPrefPath(APP_ORG, APP_NAME);   /* the data disk */
        app.menu = menu_create(&app.ga, app.campaigns, pref);
        SDL_free(pref);
        if (app.menu) { menu_start(app.menu); app.in_menu = true; card_to_display(&app); }
    }
    update_title(&app);

    bool running = true;
    Uint64 next_frame = SDL_GetTicksNS();
    while (running) {
        bool want_text = app.in_menu && app.menu && menu_wants_text(app.menu);   /* the scroll editor */
        if (want_text != SDL_TextInputActive(app.win)) {
            if (want_text) SDL_StartTextInput(app.win); else SDL_StopTextInput(app.win);
        }
        const Uint64 frame_ns = SDL_NS_PER_SECOND / (app.in_battle ? BATTLE_HZ : app.in_menu ? MENU_HZ : PLAY_HZ);
        SDL_Event e;
        bool dirty = false;
        bool got;
        if (app.playing || app.in_menu) {
            Uint64 now = SDL_GetTicksNS();
            if (now >= next_frame) {
                next_frame = (now - next_frame > frame_ns * 4) ? now + frame_ns : next_frame + frame_ns;
                if (app.in_menu) { if (!menu_step(&app)) running = false; }
                else if (!play_step(&app)) update_title(&app);
                render(&app);
                continue;
            }
            got = SDL_WaitEventTimeout(&e, (Sint32)((next_frame - now) / SDL_NS_PER_MS) + 1);
            if (!got) continue;
        } else {
            next_frame = SDL_GetTicksNS();
            if (!SDL_WaitEventTimeout(&e, 500)) { render(&app); continue; }
        }
        do {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (e.type == app.ev_folder) {
                if (load_from(&app, app.chosen, true)) {
                    app.index = 0;
                    if (!app.menu && !viewer && app.ga.have_menus) {
                        char *pref = SDL_GetPrefPath(APP_ORG, APP_NAME);
                        app.menu = menu_create(&app.ga, app.campaigns, pref);
                        SDL_free(pref);
                        if (app.menu) { menu_start(app.menu); app.in_menu = true; card_to_display(&app); }
                    }
                } else ask_for_folder(&app);
                dirty = true;
            } else if (e.type == SDL_EVENT_KEY_DOWN) {
                SDL_Keycode k = e.key.key;
                bool mod = (e.key.mod & (SDL_KMOD_ALT | SDL_KMOD_GUI)) != 0;
                if (k == SDLK_Q && (e.key.mod & SDL_KMOD_GUI)) running = false;
                else if (k == SDLK_RETURN && mod) toggle_fullscreen(&app);
                else if (app.in_menu) {
                    if (k == SDLK_TAB) next_mode(&app);
                    else if (k == SDLK_F2) app.aspect_43 = !app.aspect_43;
                    else if (k == SDLK_F12) { app.in_menu = false; app.index = 0; }
                    else {
                        int pk = k == SDLK_ESCAPE ? PK_ESC : play_keycode(k);
                        if (menu_wants_held(app.menu) && is_cursor_pk(pk)) {   /* held keys move the cursor */
                            if (e.key.repeat) continue;
                            app.tapped = pk;
                        }
                        if (pk) {
                            int before = menu_options(app.menu)->display;
                            menu_key(app.menu, pk);
                            display_to_card(&app, before);   /* the Options Shoppe's tv/composite or rgb */
                        }
                    }
                }
                else if (app.playing) {
                    if (e.key.repeat) continue;
                    bool own = k == SDLK_TAB || k == SDLK_F2;   /* the remake's own keys */
                    if (app.play.demo && !own) { end_demo(&app); update_title(&app); }   /* a key ends the demo */
                    else if (k == SDLK_TAB) next_mode(&app);
                    else if (k == SDLK_F2) app.aspect_43 = !app.aspect_43;
                    else if (app.in_battle) {
                        int pk = play_keycode(k);
                        if (pk) battle_key(&app.battle, pk);
                    } else {
                        int pk = k == SDLK_ESCAPE ? PK_ESC : play_keycode(k);
                        if (pk == PK_S && (e.key.mod & SDL_KMOD_ALT)) pk = PK_ALT_S;   /* Alt+S: save */
                        if (is_cursor_pk(pk)) app.tapped = pk;
                        else if (pk) play_key(&app.play, pk, (e.key.mod & SDL_KMOD_SHIFT) != 0);
                    }
                }
                else if (k == SDLK_ESCAPE) { if (app.menu) { menu_title(app.menu); app.in_menu = true; } else running = false; }
                else if (k == SDLK_F) toggle_fullscreen(&app);
                else if (!app.have_assets && k == SDLK_O) ask_for_folder(&app);
                else if (app.have_assets) {
                    bool shift = (e.key.mod & SDL_KMOD_SHIFT) != 0;
                    bool up = k == SDLK_UP || k == SDLK_MINUS || k == SDLK_KP_MINUS;
                    bool down = k == SDLK_DOWN || k == SDLK_EQUALS || k == SDLK_PLUS || k == SDLK_KP_PLUS;
                    if (is_map(&app) && (k == SDLK_RETURN || k == SDLK_KP_ENTER)) start_play(&app);
                    else if (is_map(&app) && (up || down)) scroll_map(&app, (shift ? MAP_PIXEL_H : TILE_H) * (up ? -1 : 1));
                    else if (is_map(&app) && k == SDLK_HOME) scroll_map(&app, -MAP_PIXEL_H);
                    else if (is_map(&app) && k == SDLK_END) scroll_map(&app, MAP_PIXEL_H);
                    else if (k == SDLK_RIGHT || k == SDLK_DOWN || k == SDLK_SPACE) app.index = (app.index + 1) % NUM_SCREENS;
                    else if (k == SDLK_LEFT || k == SDLK_UP) app.index = (app.index + NUM_SCREENS - 1) % NUM_SCREENS;
                    else if (k == SDLK_HOME) app.index = 0;
                    else if (k == SDLK_TAB) next_mode(&app);
                    else if (k == SDLK_A) app.aspect_43 = !app.aspect_43;
                    else if (k == SDLK_H) app.caption = !app.caption;
                    else if (k == SDLK_O) ask_for_folder(&app);
                }
                dirty = true;
            } else if (e.type == SDL_EVENT_TEXT_INPUT && app.in_menu && app.menu) {
                menu_text(app.menu, e.text.text);   /* the scroll editor */
                dirty = true;
            } else if (e.type == SDL_EVENT_MOUSE_WHEEL && app.have_assets && !app.playing) {
                scroll_map(&app, (int)SDL_roundf(-e.wheel.y * TILE_H / 2));
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

    gamelog_end(NULL, "The remake was closed.");
    sound_quit();
    menu_destroy(app.menu);
    if (app.have_assets) assets_free(&app.ga);
    SDL_DestroyTexture(app.up);
    SDL_DestroyTexture(app.tex);
    SDL_DestroyTexture(app.tex_herc);
    SDL_DestroyTexture(app.tex_comp);
    SDL_DestroyRenderer(app.ren);
    SDL_DestroyWindow(app.win);
    SDL_Quit();
    return 0;
}
