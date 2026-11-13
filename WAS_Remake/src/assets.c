/*
 * The Ancient Art of War at Sea (1987) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * assets.c: decoders for the original CGA and 16-colour graphics.
 *
 * Formats (see WAS_Analysis/WAS_file_reference.md in the repository):
 *   SM          94 records: word width in words, word height/2, 2bpp pixels
 *   SP*, AD, B  two fields (even lines, odd lines); each word N, then N codes:
 *               c <= 200 repeat the next byte c times, c > 200 copy c - 200 bytes
 *   WAS.EXE     data segment = the overlay table entry flagged 4000h; the
 *               8x8 font at DS:089C, the map sprites at DS:049B-05CA
 *               (palette 1, but the scrolls SPE and SPF in palette 0)
 *   WAST.DAT    archive as WARET.DAT; records = word size + signed RLE, no
 *               palette (the standard 16 colours)
 */
#include "assets.h"

#include <SDL3/SDL.h>

const RGB CGA_PALETTE[4] = {
    { 0x00, 0x00, 0x00 }, { 0x55, 0xFF, 0xFF }, { 0xFF, 0x55, 0xFF }, { 0xFF, 0xFF, 0xFF }
};

const RGB CGA_PALETTE_0[4] = {
    { 0x00, 0x00, 0x00 }, { 0x55, 0xFF, 0x55 }, { 0xFF, 0x55, 0x55 }, { 0xFF, 0xFF, 0x55 }
};

const RGB STD16_PALETTE[16] = {
    { 0x00, 0x00, 0x00 }, { 0x00, 0x00, 0xAA }, { 0x00, 0xAA, 0x00 }, { 0x00, 0xAA, 0xAA },
    { 0xAA, 0x00, 0x00 }, { 0xAA, 0x00, 0xAA }, { 0xAA, 0x55, 0x00 }, { 0xAA, 0xAA, 0xAA },
    { 0x55, 0x55, 0x55 }, { 0x55, 0x55, 0xFF }, { 0x55, 0xFF, 0x55 }, { 0x55, 0xFF, 0xFF },
    { 0xFF, 0x55, 0x55 }, { 0xFF, 0x55, 0xFF }, { 0xFF, 0xFF, 0x55 }, { 0xFF, 0xFF, 0xFF },
};

static const char *PIC_NAMES[PIC_COUNT] = {
    "Title page", "Title page with the ship", "Scroll 1", "Scroll 2", "Opponents",
    "Scenes 1", "Scenes 2", "Ye Olde Options Shoppe",
    "Battle report 1 (cannon)", "Battle report 2 (boarding)", "Battle report 3 (boarding)",
    "Battle report 4 (boarding)", "The face (opening)", "Boarding: the deck",
};

/* CGA file name and 16-colour (WAST.DAT) record for each picture; 0 = none */
static const char *CGA_FILES[PIC_COUNT] = {
    "SPM", "SPI", "SPE", "SPF", "SPJ", "SPG", "SPH", "SPK", "SPN", "SPO", "SPP", "SPQ", "AD", "B",
};
static const int REC16[PIC_COUNT] = { 1, 2, 3, 4, 5, 6, 7, 8, 10, 9, 0, 0, 13, 29 };
static const int REC16_TILES[2] = { 30, 31 };   /* tiles 1-90, 91-94 */

/* draw_map_sprite's sprites (rt:10D2 draws them; ovl4:B35E fills the heading table) */
static const uint16_t MSPR_OFFSET[MSPR_COUNT] = {
    0x049B, 0x04AD,
    0x04BF, 0x04D1, 0x04E3, 0x04F1, 0x04FF, 0x0509, 0x0517, 0x0525,
    0x0537, 0x0549, 0x055B, 0x0569, 0x0577, 0x0581, 0x058F, 0x059D,
    0x05AF, 0x05BD,
};
#define DS_HEADING_DY 0x075A

const char *picture_name(PictureId id) { return (id >= 0 && id < PIC_COUNT) ? PIC_NAMES[id] : "?"; }
const char *picture_file(PictureId id) { return (id >= 0 && id < PIC_COUNT) ? CGA_FILES[id] : "?"; }
const char *mode_name(GfxMode mode)    { return mode == MODE_16 ? "Tandy" : "CGA"; }

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

static bool image_alloc(Image *img, int w, int h, const RGB *pal, int ncolors)
{
    img->w = w;
    img->h = h;
    img->ncolors = ncolors;
    for (int i = 0; i < 16; i++) img->pal[i] = i < ncolors ? pal[i] : (RGB){ 0, 0, 0 };
    img->px = SDL_calloc((size_t)w * (size_t)h, 1);
    return img->px != NULL;
}

static void image_free(Image *img) { SDL_free(img->px); img->px = NULL; }

/* 2 bits per pixel, leftmost pixel in the top bits */
static uint8_t px2(const uint8_t *row, int x) { return (row[x >> 2] >> (6 - 2 * (x & 3))) & 3; }
/* 4 bits per pixel, leftmost pixel in the high nibble */
static uint8_t px4(const uint8_t *row, int x) { return (x & 1) ? (row[x >> 1] & 15) : (row[x >> 1] >> 4); }

/* ------------------------------------------------------------------ WAS.EXE: data segment, font, sprites */

/* The overlay table follows the resident code, the root (51Eh bytes) and the
 * loader's data (172h bytes): image offset CS*16 + 690h, where CS is the
 * entry segment. The data segment is the entry flagged 4000h (preload). */
static bool load_exe(GameAssets *ga, char *err, size_t errlen)
{
    Blob exe;
    if (!load_blob(ga->dir, "WAS.EXE", &exe)) { seterr(err, errlen, "%s not found", "WAS.EXE"); return false; }
    bool ok = false;
    const uint8_t *d = exe.data;
    if (exe.size > 0x40 && d[0] == 'M' && d[1] == 'Z') {
        size_t img = (size_t)rd16(d + 8) * 16;
        size_t tbl = img + (size_t)rd16(d + 22) * 16 + 0x690;
        for (int k = 0; tbl + (size_t)(k + 1) * 16 <= exe.size && k < 32; k++) {
            const uint8_t *e = d + tbl + (size_t)k * 16;
            uint16_t flags = rd16(e + 4);
            if (flags == 0xFFFF) break;
            if (!(flags & 0x4000)) continue;
            size_t ds = (size_t)rd16(e + 14) * 16;
            if (ds + DS_SIZE > exe.size) break;
            SDL_memcpy(ga->ds, d + ds, DS_SIZE);
            if (img + RND_CODE <= exe.size) SDL_memcpy(ga->code, d + img, RND_CODE);   /* rt is the image's first segment */
            SDL_memcpy(ga->font, ga->ds + DS_FONT, sizeof ga->font);
            /* sanity: space is empty, 'A' is not, the sprites are 2 bytes wide */
            int blank = 0, a = 0;
            for (int r = 0; r < 8; r++) { blank |= ga->font[0][r]; a |= ga->font['A' - FONT_FIRST][r]; }
            ok = blank == 0 && a != 0;
            for (int s = 0; ok && s < MSPR_COUNT; s++) {
                const uint8_t *p = ga->ds + MSPR_OFFSET[s];
                int wb = p[0], hp = p[1];
                ok = wb == 2 && hp >= 1 && hp * 2 <= MSPR_MAX_H;
                if (!ok) break;
                Sprite *sp = &ga->map_sprite[s];
                sp->w = wb * 4;
                sp->h = hp * 2;
                p += 2;
                for (int y = 0; y < sp->h; y++, p += wb)   /* per line pair: the even line, then the odd line */
                    for (int x = 0; x < sp->w; x++) sp->px[y][x] = px2(p, x);
            }
            for (int h = 0; ok && h < 16; h++) ga->heading_dy[h] = ga->ds[DS_HEADING_DY + h] == 0x50 ? 1 : 0;
            break;
        }
    }
    free_blob(&exe);
    if (!ok) seterr(err, errlen, "%s is not a version this remake recognises", "WAS.EXE");
    return ok;
}

/* ------------------------------------------------------------------ CGA */

bool assets_decode_picture(const uint8_t *data, size_t size, uint8_t fields[16000])
{
    size_t pos = 0;
    for (int f = 0; f < 2; f++) {
        uint8_t *out = fields + f * 8000;
        size_t o = 0;
        if (pos + 2 > size) return false;
        unsigned n = rd16(data + pos);
        pos += 2;
        for (unsigned i = 0; i < n; i++) {
            if (pos >= size) return false;
            unsigned c = data[pos++];
            if (c <= 200) {                       /* a run */
                if (pos >= size || o + c > 8000) return false;
                SDL_memset(out + o, data[pos++], c);
                o += c;
            } else {                              /* c - 200 bytes as they are */
                c -= 200;
                if (pos + c > size || o + c > 8000) return false;
                SDL_memcpy(out + o, data + pos, c);
                pos += c;
                o += c;
            }
        }
        if (o != 8000) return false;
    }
    return true;
}

static void cga_fields_to_image(const uint8_t *fields, Image *img)
{
    for (int y = 0; y < SCREEN_H; y++) {
        const uint8_t *row = fields + (y & 1) * 8000 + (y >> 1) * 80;
        for (int x = 0; x < SCREEN_W; x++) img->px[y * SCREEN_W + x] = px2(row, x);
    }
}

static bool load_cga(GameAssets *ga, char *err, size_t errlen)
{
    AssetSet *set = &ga->set[MODE_CGA];
    uint8_t *fields = SDL_malloc(16000);
    if (!fields) return false;
    for (int i = 0; i < PIC_COUNT; i++) {
        Blob b;
        if (!load_blob(ga->dir, CGA_FILES[i], &b)) { seterr(err, errlen, "%s not found", CGA_FILES[i]); SDL_free(fields); return false; }
        bool ok = assets_decode_picture(b.data, b.size, fields);
        free_blob(&b);
        if (!ok) { seterr(err, errlen, "%s could not be decoded", CGA_FILES[i]); SDL_free(fields); return false; }
        Image *img = &set->pictures[i];
        /* show_scroll (ovl1:7F1A) shows the scrolls in palette 0, like WAR.EXE */
        const RGB *pal = (i == PIC_SCROLL_1 || i == PIC_SCROLL_2) ? CGA_PALETTE_0 : CGA_PALETTE;
        if (!image_alloc(img, SCREEN_W, SCREEN_H, pal, 4)) { SDL_free(fields); return false; }
        cga_fields_to_image(fields, img);
    }
    SDL_free(fields);

    Blob m;
    if (!load_blob(ga->dir, "SM", &m)) { seterr(err, errlen, "%s not found", "SM"); return false; }
    size_t pos = 0;
    for (int t = 0; t < NUM_TILES; t++) {
        if (pos + 4 > m.size) break;
        int bpr = rd16(m.data + pos) * 2, rows = rd16(m.data + pos + 2) * 2;
        pos += 4;
        if (bpr * 4 != TILE_W || rows != TILE_H || pos + (size_t)(bpr * rows) > m.size) break;
        Image *img = &set->tiles[t];
        if (!image_alloc(img, TILE_W, TILE_H, CGA_PALETTE, 4)) break;
        for (int y = 0; y < rows; y++)
            for (int x = 0; x < TILE_W; x++) img->px[y * TILE_W + x] = px2(m.data + pos + y * bpr, x);
        pos += (size_t)(bpr * rows);
    }
    free_blob(&m);
    if (!set->tiles[NUM_TILES - 1].px) { seterr(err, errlen, "%s could not be decoded", "SM"); return false; }
    set->loaded = true;
    return true;
}

/* ------------------------------------------------------------------ 16 colours (WAST.DAT) */

bool assets_unpack16(const uint8_t *src, size_t srclen, uint8_t *out, size_t size)
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

/* The payload of a record (checksum checked), or NULL. */
static const uint8_t *dat_record(const Blob *dat, int id, size_t *len)
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

/* Word size + RLE (no palette in WAST.DAT). Returns malloc'd unpacked data. */
static uint8_t *dat_unpack(const Blob *dat, int id, size_t *size)
{
    size_t len;
    const uint8_t *p = dat_record(dat, id, &len);
    if (!p || len < 3) return NULL;
    *size = rd16(p);
    uint8_t *out = SDL_malloc(*size ? *size : 1);
    if (out && !assets_unpack16(p + 2, len - 2, out, *size)) { SDL_free(out); out = NULL; }
    return out;
}

static bool load_16(GameAssets *ga)
{
    Blob dat;
    if (!load_blob(ga->dir, "WAST.DAT", &dat)) return false;
    AssetSet *set = &ga->set[MODE_16];
    bool ok = true;
    for (int i = 0; i < PIC_COUNT && ok; i++) {
        if (!REC16[i]) continue;                 /* SPP, SPQ: not in the Tandy version */
        size_t size = 0;
        uint8_t *raw = dat_unpack(&dat, REC16[i], &size);
        ok = raw && size >= 6;
        if (ok) {
            int w = rd16(raw), h = rd16(raw + 2), bpr = rd16(raw + 4);
            ok = w == SCREEN_W && h == SCREEN_H && bpr * 2 >= w && size >= 6 + (size_t)bpr * (size_t)h;
            Image *img = &set->pictures[i];
            if (ok && image_alloc(img, w, h, STD16_PALETTE, 16)) {
                for (int y = 0; y < h; y++)
                    for (int x = 0; x < w; x++) img->px[y * w + x] = px4(raw + 6 + (size_t)y * bpr, x);
            }
        }
        SDL_free(raw);
    }
    int t = 0;
    for (int r = 0; r < 2 && ok; r++) {
        size_t size = 0, pos = 0;
        uint8_t *raw = dat_unpack(&dat, REC16_TILES[r], &size);
        ok = raw != NULL;
        while (ok && pos + 4 <= size && t < NUM_TILES) {
            int bpr = rd16(raw + pos) * 2, rows = rd16(raw + pos + 2) * 2;
            pos += 4;
            if (bpr * 2 != TILE_W || rows != TILE_H || pos + (size_t)(bpr * rows) > size) { ok = false; break; }
            Image *img = &set->tiles[t];
            if (!image_alloc(img, TILE_W, TILE_H, STD16_PALETTE, 16)) { ok = false; break; }
            for (int y = 0; y < rows; y++)
                for (int x = 0; x < TILE_W; x++) img->px[y * TILE_W + x] = px4(raw + pos + (size_t)y * bpr, x);
            pos += (size_t)(bpr * rows);
            t++;
        }
        SDL_free(raw);
    }
    ok = ok && t == NUM_TILES;
    free_blob(&dat);
    if (!ok) {
        for (int i = 0; i < PIC_COUNT; i++) image_free(&set->pictures[i]);
        for (int k = 0; k < NUM_TILES; k++) image_free(&set->tiles[k]);
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
    return dir && *dir && find_file(dir, "WAS.EXE", path, sizeof path);
}

const char *assets_ds_string(const GameAssets *ga, unsigned offset)
{
    if (offset >= DS_SIZE) return "";
    for (unsigned i = offset; i < DS_SIZE; i++)
        if (ga->ds[i] == 0) return (const char *)ga->ds + offset;
    return "";
}

const char *assets_buf_string(const uint8_t *buf, size_t size, unsigned offset)
{
    if (!buf || offset >= size) return "";
    for (size_t i = offset; i < size; i++)
        if (buf[i] == 0) return (const char *)buf + offset;
    return "";
}

const char *assets_so5_string(const GameAssets *ga, unsigned offset)
{
    if (offset >= sizeof ga->so5 - 1) return "";
    return (const char *)ga->so5 + offset;   /* so5[] ends with a zero */
}

bool assets_load(GameAssets *ga, const char *dir, char *err, size_t errlen)
{
    SDL_memset(ga, 0, sizeof *ga);
    SDL_strlcpy(ga->dir, dir, sizeof ga->dir);
    if (!load_exe(ga, err, errlen) || !load_cga(ga, err, errlen)) {
        assets_free(ga);
        return false;
    }
    load_16(ga);   /* optional: WAST.DAT */
    /* SO5: the messages of campaign play (campaign_play loads 373h bytes to DS:4AF5) */
    size_t n = 0;
    uint8_t *so5 = assets_load_file(dir, "SO5", &n);
    ga->have_so5 = so5 && n >= 0x2C9;
    if (so5) SDL_memcpy(ga->so5, so5, SDL_min(n, sizeof ga->so5 - 1));
    SDL_free(so5);
    /* SO2: the opponents' values (copied to DS:604D when one is chosen, ovl1:90F7) */
    uint8_t *so2 = assets_load_file(dir, "SO2", &n);
    if (so2 && n >= 0x277 + 6 * OPP_WORDS * 2) {
        for (int k = 0; k < 6; k++)
            for (int i = 0; i < OPP_WORDS; i++) ga->opp[k][i] = (int16_t)rd16(so2 + 0x277 + k * OPP_WORDS * 2 + i * 2);
        ga->have_so2 = true;
    }
    SDL_free(so2);
    /* SCNF: the options (ovl1_startup sets these defaults, then reads 29 bytes to DS:684C) */
    static const uint8_t SCNF_DEFAULT[SCNF_SIZE] = {
        0x2D, 0x01, 0x00, 0x02, 0x00, 0xF4, 0x01,   /* 684C, 684D = 1, sound 684F = 2, fight delay 6851 = 500 */
        [0x6868 - 0x684C] = 1,                      /* the wind 6865 off */
    };
    SDL_memcpy(ga->scnf, SCNF_DEFAULT, sizeof ga->scnf);
    /* the menus' files (ovl1: load_so2, load_o3, SO4 in ovl1_82af, SITLES) */
    struct { const char *name; uint8_t *to; size_t size, min; } mf[5] = {
        { "SO2", ga->so2, sizeof ga->so2, 0x14D3 }, { "SO3", ga->so3, sizeof ga->so3, 0xEEE },
        { "SO4", ga->so4, sizeof ga->so4, 0x1FB }, { "SITLES", ga->sitles, sizeof ga->sitles, 876 },
        { "SDITLES", ga->sditles, sizeof ga->sditles, 876 },
    };
    ga->have_menus = true;
    for (int k = 0; k < 5; k++) {
        uint8_t *d = assets_load_file(dir, mf[k].name, &n);
        if (!d || n < mf[k].min) ga->have_menus = false;
        if (d) SDL_memcpy(mf[k].to, d, SDL_min(n, mf[k].size));
        SDL_free(d);
    }
    uint8_t *scnf = assets_load_file(dir, "SCNF", &n);
    if (scnf && n >= SCNF_SIZE) SDL_memcpy(ga->scnf, scnf, SCNF_SIZE);
    SDL_free(scnf);
    return true;
}

int assets_scnf_word(const GameAssets *ga, unsigned ds_offset)
{
    if (ds_offset < 0x684C || ds_offset + 2 > 0x684C + SCNF_SIZE) return 0;
    return (int16_t)rd16(ga->scnf + ds_offset - 0x684C);
}

void assets_free(GameAssets *ga)
{
    for (int m = 0; m < MODE_COUNT; m++) {
        for (int i = 0; i < PIC_COUNT; i++) image_free(&ga->set[m].pictures[i]);
        for (int t = 0; t < NUM_TILES; t++) image_free(&ga->set[m].tiles[t]);
        ga->set[m].loaded = false;
    }
}
