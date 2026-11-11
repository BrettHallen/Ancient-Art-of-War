/*
 * The Ancient Art of War - remake
 * assets.c: decoders for the original CGA and VGA graphics.
 *
 * Formats (see WAR_Analysis/README.md in the repository):
 *   M            72 records: word width-in-words, word height/2, 2bpp pixels
 *   P0-PD, PL    word RLE, two fields (even lines, odd lines), 8000 bytes each
 *   PE-PO        byte RLE, runs alternate between the even and odd fields
 *   WAR.EXE      8x8 font at DS:05C9 in the data segment (DGROUP overlay)
 *   WARET.DAT    archive; pictures = 48-byte palette, word size, signed RLE
 */
#include "assets.h"

#include <SDL3/SDL.h>
#include <string.h>

const RGB CGA_PALETTE[4] = {
    { 0x00, 0x00, 0x00 }, { 0x55, 0xFF, 0xFF }, { 0xFF, 0x55, 0xFF }, { 0xFF, 0xFF, 0xFF }
};

static const char *PIC_NAMES[PIC_COUNT] = {
    "Title screen", "Title screen with barbarian", "Scroll 1", "Scroll 2", "Opponents",
    "Scenes 1 (campaign menu)", "Scenes 2 (editor menu)", "Ye Olde Options Shoppe",
    "Battle report: victory", "Battle report: defeat", "Formation editor",
    "Battle: inside a fort", "Battle: fort gate", "Battle: fort gate (other side)",
    "Battle: village", "Battle: hills", "Battle: hills (palm)", "Battle: river",
    "Battle: forest", "Battle: forest 2", "Battle: mountains", "Battle: bridge",
    "Battle: plains", "Battle: plains 2", "Battle: plains (beach)",
};

/* CGA file name and VGA (WARET.DAT) record for each picture */
static const char *CGA_FILES[PIC_COUNT] = {
    "PM", "PI", "PE", "PF", "PJ", "PG", "PH", "PK", "PN", "PO", "PL",
    "P0", "P1", "P2", "P3", "P4", "P5", "P6", "P7", "P8", "P9", "PA", "PB", "PC", "PD",
};
static const int VGA_RECORDS[PIC_COUNT] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
    60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73,
};
#define VGA_TILES_RECORD 30

const char *picture_name(PictureId id) { return (id >= 0 && id < PIC_COUNT) ? PIC_NAMES[id] : "?"; }
const char *mode_name(GfxMode mode)    { return mode == MODE_VGA ? "VGA" : "CGA"; }

/* ------------------------------------------------------------------ helpers */

typedef struct { uint8_t *data; size_t size; } Blob;

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static uint32_t rd32(const uint8_t *p) { return (uint32_t)rd16(p) | ((uint32_t)rd16(p + 2) << 16); }

static void seterr(char *err, size_t n, const char *fmt, const char *arg)
{
    if (err && n) SDL_snprintf(err, n, fmt, arg);
}

/* Find a file in dir, ignoring case (DOS names are upper case). */
static bool find_file(const char *dir, const char *name, char *out, size_t outlen)
{
    int count = 0;
    char **found = SDL_GlobDirectory(dir, name, SDL_GLOB_CASEINSENSITIVE, &count);
    bool ok = false;
    if (found && count > 0) {
        size_t dl = SDL_strlen(dir);
        const char *sep = (dl > 0 && (dir[dl - 1] == '/' || dir[dl - 1] == '\\')) ? "" : "/";
        SDL_snprintf(out, outlen, "%s%s%s", dir, sep, found[0]);
        ok = true;
    }
    SDL_free(found);
    return ok;
}

static bool load_blob(const char *dir, const char *name, Blob *b)
{
    char path[2048];
    b->data = NULL;
    b->size = 0;
    if (!find_file(dir, name, path, sizeof path)) return false;
    b->data = SDL_LoadFile(path, &b->size);
    return b->data != NULL;
}

static void free_blob(Blob *b) { SDL_free(b->data); b->data = NULL; b->size = 0; }

static bool image_alloc(Image *img, int w, int h, int ncolors)
{
    img->w = w;
    img->h = h;
    img->ncolors = ncolors;
    img->px = SDL_calloc((size_t)w * (size_t)h, 1);
    return img->px != NULL;
}

static void image_free(Image *img) { SDL_free(img->px); img->px = NULL; }

static void set_cga_palette(Image *img)
{
    for (int i = 0; i < 4; i++) img->pal[i] = CGA_PALETTE[i];
}

/* 2 bits per pixel, leftmost pixel in the top bits */
static uint8_t px2(const uint8_t *row, int x) { return (row[x >> 2] >> (6 - 2 * (x & 3))) & 3; }
/* 4 bits per pixel, leftmost pixel in the high nibble */
static uint8_t px4(const uint8_t *row, int x) { return (x & 1) ? (row[x >> 1] & 15) : (row[x >> 1] >> 4); }

/* ------------------------------------------------------------------ WAR.EXE font and sprites */

/* Map sprite in the data segment: word width (1 = 8 pixels), word height in
 * line pairs (6), then for each pair a big-endian 2bpp word for the even line
 * and one for the odd line. The sprites only use colours 0 and 3. */
static void decode_sprite(const uint8_t *p, uint8_t out[SPRITE_H][SPRITE_W])
{
    p += 4;
    for (int y = 0; y < SPRITE_H; y++, p += 2) {
        unsigned v = ((unsigned)p[0] << 8) | p[1];
        for (int x = 0; x < SPRITE_W; x++) out[y][x] = ((v >> (14 - 2 * x)) & 3) ? 1 : 0;
    }
}

/* The overlay table (image offset 2F60h) lists the data segment as the entry
 * flagged 4000h (preload). The font is at offset 05C9h in that segment, the
 * squad sprites at 031Ch (28 bytes each) and the flag at 03A8h. */
static bool load_font(GameAssets *ga, char *err, size_t errlen)
{
    Blob exe;
    if (!load_blob(ga->dir, "WAR.EXE", &exe)) { seterr(err, errlen, "%s not found", "WAR.EXE"); return false; }
    bool ok = false;
    const uint8_t *d = exe.data;
    if (exe.size > 0x40 && d[0] == 'M' && d[1] == 'Z') {
        size_t img = (size_t)rd16(d + 8) * 16;
        size_t tbl = img + 0x2F60;
        for (int k = 0; tbl + (size_t)(k + 2) * 16 <= exe.size && k < 32; k++) {
            const uint8_t *e = d + tbl + (size_t)k * 16;
            uint16_t flags = rd16(e + 4);
            if (flags == 0xFFFF) break;
            if (flags & 0x4000) {
                size_t ds = (size_t)rd16(e + 14) * 16;
                size_t font = ds + 0x5C9;
                if (font + FONT_COUNT * 8 <= exe.size) {
                    SDL_memcpy(ga->font, d + font, sizeof ga->font);
                    /* sanity: space is empty, 'A' is not */
                    int blank = 0, a = 0;
                    for (int r = 0; r < 8; r++) { blank |= ga->font[0][r]; a |= ga->font['A' - FONT_FIRST][r]; }
                    ok = (blank == 0 && a != 0);
                    if (ok && ds + DS_SIZE <= exe.size) SDL_memcpy(ga->ds, d + ds, DS_SIZE);
                    if (ok && img + RND_CODE <= exe.size) SDL_memcpy(ga->code, d + img, RND_CODE);
                    if (ok && ds + 0x3A8 + 28 <= exe.size) {
                        for (int k = 0; k < POSE_COUNT; k++) decode_sprite(d + ds + 0x31C + 28 * k, ga->squad_sprite[k]);
                        decode_sprite(d + ds + 0x3A8, ga->flag_sprite);
                    }
                }
                break;
            }
        }
    }
    free_blob(&exe);
    if (!ok) seterr(err, errlen, "%s is not a version this remake recognises", "WAR.EXE");
    return ok;
}

static void load_o5(GameAssets *ga);

/* ------------------------------------------------------------------ CGA */

static void cga_fields_to_image(const uint8_t *fields /* 16000 bytes */, Image *img)
{
    for (int y = 0; y < SCREEN_H; y++) {
        const uint8_t *row = fields + (y & 1) * 8000 + (y >> 1) * 80;
        for (int x = 0; x < SCREEN_W; x++) img->px[y * SCREEN_W + x] = px2(row, x);
    }
}

/* P0-PD, PL: per field, word N then N x [count, word] */
static bool decode_rle16(const Blob *b, uint8_t *fields)
{
    size_t pos = 0;
    for (int f = 0; f < 2; f++) {
        uint8_t *out = fields + f * 8000;
        size_t o = 0;
        if (pos + 2 > b->size) return false;
        unsigned n = rd16(b->data + pos);
        pos += 2;
        for (unsigned i = 0; i < n; i++) {
            if (pos + 3 > b->size) return false;
            unsigned count = b->data[pos];
            for (unsigned c = 0; c < count && o + 2 <= 8000; c++) {
                out[o++] = b->data[pos + 1];
                out[o++] = b->data[pos + 2];
            }
            pos += 3;
        }
        if (o != 8000) return false;
    }
    return true;
}

/* PE-PO: pairs of [count, byte], alternating between the even and odd field */
static bool decode_rle8(const Blob *b, uint8_t *fields)
{
    size_t pos = 0, o[2] = { 0, 0 };
    int f = 0;
    while ((o[0] < 8000 || o[1] < 8000) && pos + 2 <= b->size) {
        if (o[f] < 8000) {
            unsigned count = b->data[pos];
            for (unsigned c = 0; c < count && o[f] < 8000; c++) fields[f * 8000 + o[f]++] = b->data[pos + 1];
            pos += 2;
        }
        f ^= 1;
    }
    return o[0] == 8000 && o[1] == 8000;
}

static bool load_cga(GameAssets *ga, char *err, size_t errlen)
{
    AssetSet *set = &ga->set[MODE_CGA];
    uint8_t *fields = SDL_malloc(16000);
    if (!fields) return false;

    for (int i = 0; i < PIC_COUNT; i++) {
        Blob b;
        if (!load_blob(ga->dir, CGA_FILES[i], &b)) { seterr(err, errlen, "%s not found", CGA_FILES[i]); SDL_free(fields); return false; }
        bool rle8 = CGA_FILES[i][1] >= 'E' && CGA_FILES[i][1] != 'L';   /* PE-PO except PL */
        bool ok = rle8 ? decode_rle8(&b, fields) : decode_rle16(&b, fields);
        free_blob(&b);
        if (!ok) { seterr(err, errlen, "%s could not be decoded", CGA_FILES[i]); SDL_free(fields); return false; }
        Image *img = &set->pictures[i];
        if (!image_alloc(img, SCREEN_W, SCREEN_H, 4)) { SDL_free(fields); return false; }
        set_cga_palette(img);
        cga_fields_to_image(fields, img);
    }
    SDL_free(fields);

    Blob m;
    if (!load_blob(ga->dir, "M", &m)) { seterr(err, errlen, "%s not found", "M"); return false; }
    size_t pos = 0;
    for (int t = 0; t < NUM_TILES; t++) {
        if (pos + 4 > m.size) break;
        int bpr = rd16(m.data + pos) * 2, rows = rd16(m.data + pos + 2) * 2;
        pos += 4;
        if (bpr * 4 != TILE_W || rows != TILE_H || pos + (size_t)(bpr * rows) > m.size) break;
        Image *img = &set->tiles[t];
        if (!image_alloc(img, TILE_W, TILE_H, 4)) break;
        set_cga_palette(img);
        for (int y = 0; y < rows; y++)
            for (int x = 0; x < TILE_W; x++) img->px[y * TILE_W + x] = px2(m.data + pos + y * bpr, x);
        pos += (size_t)(bpr * rows);
    }
    free_blob(&m);
    if (!set->tiles[NUM_TILES - 1].px) { seterr(err, errlen, "%s could not be decoded", "M"); return false; }
    set->loaded = true;
    return true;
}

/* ------------------------------------------------------------------ VGA (WARET.DAT) */

/* Signed RLE used by WARET.EXE: n >= 0 copy n+1 bytes, n < 0 repeat next byte -n times. */
static bool unpack(const uint8_t *src, size_t srclen, uint8_t *out, size_t size)
{
    size_t i = 0, o = 0;
    while (o < size) {
        if (i >= srclen) return false;
        unsigned n = src[i++];
        if (n < 128) {
            for (unsigned k = 0; k <= n && o < size; k++) {
                if (i >= srclen) return false;
                out[o++] = src[i++];
            }
        } else {
            if (i >= srclen) return false;
            uint8_t v = src[i++];
            for (unsigned k = 0; k < 256 - n && o < size; k++) out[o++] = v;
        }
    }
    return true;
}

/* Returns the record payload, or NULL. */
static const uint8_t *waret_record(const Blob *dat, int id, size_t *len)
{
    if (dat->size < 8) return NULL;
    uint32_t t = rd32(dat->data);
    if ((size_t)t + 5 > dat->size) return NULL;
    unsigned count = rd16(dat->data + t + 3);
    for (unsigned k = 0; k < count; k++) {
        size_t e = (size_t)t + 5 + (size_t)k * 6;
        if (e + 6 > dat->size) return NULL;
        if (rd16(dat->data + e) != id) continue;
        size_t off = rd32(dat->data + e + 2);
        if (off + 3 > dat->size) return NULL;
        size_t l = rd16(dat->data + off);
        if (off + 3 + l > dat->size) return NULL;
        const uint8_t *p = dat->data + off + 3;
        unsigned sum = 0;
        for (size_t i = 0; i < l; i++) sum += p[i];
        if ((sum & 255) != dat->data[off + 2]) return NULL;
        *len = l;
        return p;
    }
    return NULL;
}

/* Palette (48 bytes) + word size + RLE. Returns malloc'd unpacked data. */
static uint8_t *waret_unpack_with_palette(const Blob *dat, int id, RGB pal[16], size_t *size)
{
    size_t len;
    const uint8_t *p = waret_record(dat, id, &len);
    if (!p || len < 50) return NULL;
    for (int i = 0; i < 16; i++) { pal[i].r = p[i * 3]; pal[i].g = p[i * 3 + 1]; pal[i].b = p[i * 3 + 2]; }
    *size = rd16(p + 48);
    uint8_t *out = SDL_malloc(*size ? *size : 1);
    if (out && !unpack(p + 50, len - 50, out, *size)) { SDL_free(out); out = NULL; }
    return out;
}

static bool load_vga(GameAssets *ga)
{
    Blob dat;
    if (!load_blob(ga->dir, "WARET.DAT", &dat)) return false;
    AssetSet *set = &ga->set[MODE_VGA];
    bool ok = true;

    for (int i = 0; i < PIC_COUNT && ok; i++) {
        RGB pal[16];
        size_t size;
        uint8_t *raw = waret_unpack_with_palette(&dat, VGA_RECORDS[i], pal, &size);
        ok = raw && size >= 6;
        if (ok) {
            int w = rd16(raw), h = rd16(raw + 2), bpr = rd16(raw + 4);
            ok = w == SCREEN_W && h == SCREEN_H && bpr * 2 >= w && size >= 6 + (size_t)bpr * (size_t)h;
            Image *img = &set->pictures[i];
            if (ok && image_alloc(img, w, h, 16)) {
                SDL_memcpy(img->pal, pal, sizeof pal);
                for (int y = 0; y < h; y++)
                    for (int x = 0; x < w; x++) img->px[y * w + x] = px4(raw + 6 + (size_t)y * bpr, x);
            }
        }
        SDL_free(raw);
    }

    if (ok) {
        RGB pal[16];
        size_t size, pos = 0;
        uint8_t *raw = waret_unpack_with_palette(&dat, VGA_TILES_RECORD, pal, &size);
        ok = raw != NULL;
        for (int t = 0; ok && t < NUM_TILES; t++) {
            if (pos + 4 > size) { ok = false; break; }
            int bpr = rd16(raw + pos) * 2, rows = rd16(raw + pos + 2) * 2;
            pos += 4;
            if (bpr * 2 != TILE_W || rows != TILE_H || pos + (size_t)(bpr * rows) > size) { ok = false; break; }
            Image *img = &set->tiles[t];
            if (!image_alloc(img, TILE_W, TILE_H, 16)) { ok = false; break; }
            SDL_memcpy(img->pal, pal, sizeof pal);
            for (int y = 0; y < rows; y++)
                for (int x = 0; x < TILE_W; x++) img->px[y * TILE_W + x] = px4(raw + pos + (size_t)y * bpr, x);
            pos += (size_t)(bpr * rows);
        }
        SDL_free(raw);
    }
    free_blob(&dat);

    if (!ok) {
        for (int i = 0; i < PIC_COUNT; i++) image_free(&set->pictures[i]);
        for (int t = 0; t < NUM_TILES; t++) image_free(&set->tiles[t]);
    }
    set->loaded = ok;
    return ok;
}

/* ------------------------------------------------------------------ public */

void *assets_load_file(const char *dir, const char *name, size_t *size)
{
    Blob b;
    if (!load_blob(dir, name, &b)) return NULL;
    if (size) *size = b.size;
    return b.data;
}

bool assets_dir_ok(const char *dir)
{
    char path[2048];
    return dir && *dir && find_file(dir, "WAR.EXE", path, sizeof path);
}

bool assets_load(GameAssets *ga, const char *dir, char *err, size_t errlen)
{
    SDL_memset(ga, 0, sizeof *ga);
    SDL_strlcpy(ga->dir, dir, sizeof ga->dir);
    if (!load_font(ga, err, errlen) || !load_cga(ga, err, errlen)) {
        assets_free(ga);
        return false;
    }
    load_o5(ga);
    load_vga(ga);   /* optional: only in releases that include WARET.DAT */
    return true;
}

/* O5 holds the messages shown during a campaign (WAR.EXE loads it to DS:9424). */
static void load_o5(GameAssets *ga)
{
    size_t size = 0;
    uint8_t *d = assets_load_file(ga->dir, "O5", &size);
    SDL_memset(ga->o5, 0, sizeof ga->o5);
    ga->have_o5 = d && size >= 0x35E;   /* 862 bytes; WAR.EXE reads up to 373h */
    if (ga->have_o5) SDL_memcpy(ga->o5, d, SDL_min(size, (size_t)O5_SIZE));
    SDL_free(d);

    /* O6, O7 and F: the battle screen's pictures, scripts, tables and formations */
    size_t s6 = 0, s7 = 0, sf = 0;
    uint8_t *o6 = assets_load_file(ga->dir, "O6", &s6), *o7 = assets_load_file(ga->dir, "O7", &s7),
            *f = assets_load_file(ga->dir, "F", &sf);
    ga->have_battle = o6 && o7 && f && s6 >= 0x1C8A && s7 >= 0x7EC && sf >= 420;
    if (ga->have_battle) {
        SDL_memcpy(ga->o6, o6, SDL_min(s6, sizeof ga->o6));
        SDL_memcpy(ga->o7, o7, SDL_min(s7, sizeof ga->o7));
        SDL_memcpy(ga->formations, f, 420);
    }
    SDL_free(o6); SDL_free(o7); SDL_free(f);

    /* O2: the opponents' descriptions, then 8 x 19 words of their values at 277h
     * (choose_opponent copies one set to DS:A4DC) */
    d = assets_load_file(ga->dir, "O2", &size);
    ga->have_o2 = d && size >= 0x277 + 8 * OPP_PARAMS * 2;
    for (int k = 0; ga->have_o2 && k < 8; k++)
        for (int w = 0; w < OPP_PARAMS; w++)
            ga->opponent[k][w] = (int16_t)rd16(d + 0x277 + (k * OPP_PARAMS + w) * 2);
    SDL_free(d);
}

const char *assets_ds_string(const GameAssets *ga, unsigned offset)
{
    if (offset >= DS_SIZE) return "";
    for (unsigned i = offset; i < DS_SIZE; i++)
        if (ga->ds[i] == 0) return (const char *)ga->ds + offset;
    return "";
}

const char *assets_o5_string(const GameAssets *ga, unsigned offset)
{
    if (!ga->have_o5 || offset >= O5_SIZE) return "";   /* o5[O5_SIZE] is always 0 */
    return (const char *)ga->o5 + offset;
}

int assets_ds_bitmap_px(const GameAssets *ga, unsigned offset, int x, int y, int *w, int *h)
{
    if (offset + 4 > DS_SIZE) return 0;
    int ww = rd16(ga->ds + offset) * 8, hh = rd16(ga->ds + offset + 2) * 2;
    if (w) *w = ww;
    if (h) *h = hh;
    if (x < 0 || y < 0 || x >= ww || y >= hh) return 0;
    unsigned at = offset + 4 + (unsigned)(y * (ww / 4) + x / 4);
    if (at >= DS_SIZE) return 0;
    return (ga->ds[at] >> (6 - 2 * (x & 3))) & 3;
}

void assets_free(GameAssets *ga)
{
    for (int m = 0; m < MODE_COUNT; m++) {
        for (int i = 0; i < PIC_COUNT; i++) image_free(&ga->set[m].pictures[i]);
        for (int t = 0; t < NUM_TILES; t++) image_free(&ga->set[m].tiles[t]);
        ga->set[m].loaded = false;
    }
}
