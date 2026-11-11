/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * make_icon: writes the app icon as a macOS .iconset (PNG files) from the
 * player's own game files, at build time, so no original artwork is kept
 * in the repository. CMake then runs iconutil to make AppIcon.icns.
 *
 * Usage: make_icon GAME-FOLDER OUT.iconset
 */
#include <SDL3/SDL.h>

#include "../src/assets.h"
#include "../src/icon.h"

/* ------------------------------------------------------------------ a small PNG writer (stored deflate) */

static uint32_t crc_table[256];

static void crc_init(void)
{
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        crc_table[n] = c;
    }
}

static uint32_t crc(uint32_t c, const uint8_t *p, size_t n)
{
    for (size_t i = 0; i < n; i++) c = crc_table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    return c;
}

static void put32(uint8_t *p, uint32_t v) { p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v; }

static bool chunk(SDL_IOStream *io, const char *type, const uint8_t *data, uint32_t n)
{
    uint8_t h[8];
    put32(h, n);
    SDL_memcpy(h + 4, type, 4);
    uint32_t c = crc(0xFFFFFFFFu, (const uint8_t *)type, 4);
    c = crc(c, data, n) ^ 0xFFFFFFFFu;
    uint8_t t[4];
    put32(t, c);
    return SDL_WriteIO(io, h, 8) == 8 && (n == 0 || SDL_WriteIO(io, data, n) == n) && SDL_WriteIO(io, t, 4) == 4;
}

static bool write_png(const char *path, const uint8_t *rgba, int w, int h)
{
    size_t row = (size_t)w * 4 + 1, raw_n = row * h;
    uint8_t *raw = SDL_malloc(raw_n);
    size_t blocks = (raw_n + 65534) / 65535;
    size_t z_n = 2 + raw_n + blocks * 5 + 4;
    uint8_t *z = SDL_malloc(z_n);
    if (!raw || !z) { SDL_free(raw); SDL_free(z); return false; }
    for (int y = 0; y < h; y++) {
        raw[y * row] = 0;                                  /* no filter */
        SDL_memcpy(raw + y * row + 1, rgba + (size_t)y * w * 4, (size_t)w * 4);
    }
    uint8_t *p = z;
    *p++ = 0x78; *p++ = 0x01;                              /* zlib header */
    uint32_t a = 1, b = 0;
    for (size_t off = 0; off < raw_n; off += 65535) {
        size_t n = SDL_min((size_t)65535, raw_n - off);
        *p++ = off + n >= raw_n;                           /* stored block, last flag */
        *p++ = (uint8_t)n; *p++ = (uint8_t)(n >> 8);
        *p++ = (uint8_t)~n; *p++ = (uint8_t)(~n >> 8);
        SDL_memcpy(p, raw + off, n);
        for (size_t i = 0; i < n; i++) { a = (a + raw[off + i]) % 65521; b = (b + a) % 65521; }
        p += n;
    }
    put32(p, (b << 16) | a);
    p += 4;
    uint8_t ihdr[13];
    put32(ihdr, (uint32_t)w);
    put32(ihdr + 4, (uint32_t)h);
    ihdr[8] = 8; ihdr[9] = 6; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;   /* 8-bit RGBA */
    SDL_IOStream *io = SDL_IOFromFile(path, "wb");
    bool ok = io && SDL_WriteIO(io, "\x89PNG\r\n\x1a\n", 8) == 8 &&
              chunk(io, "IHDR", ihdr, 13) && chunk(io, "IDAT", z, (uint32_t)(p - z)) && chunk(io, "IEND", NULL, 0);
    if (io) SDL_CloseIO(io);
    SDL_free(raw);
    SDL_free(z);
    return ok;
}

/* ------------------------------------------------------------------ main */

int main(int argc, char **argv)
{
    if (argc < 3) { SDL_Log("usage: make_icon GAME-FOLDER OUT.iconset"); return 2; }
    static GameAssets ga;
    char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) { SDL_Log("make_icon: %s", err); return 1; }
    if (!SDL_CreateDirectory(argv[2])) { SDL_Log("make_icon: %s", SDL_GetError()); return 1; }
    crc_init();
    static const int SIZES[5] = { 16, 32, 128, 256, 512 };
    uint8_t *rgba = SDL_malloc(1024 * 1024 * 4);
    for (int k = 0; k < 5; k++)
        for (int scale = 1; scale <= 2; scale++) {
            int n = SIZES[k] * scale;
            if (!icon_render(&ga, n, rgba)) { SDL_Log("make_icon: no title picture in %s", argv[1]); return 1; }
            char path[1024];
            SDL_snprintf(path, sizeof path, "%s/icon_%dx%d%s.png", argv[2], SIZES[k], SIZES[k], scale == 2 ? "@2x" : "");
            if (!write_png(path, rgba, n, n)) { SDL_Log("make_icon: can't write %s", path); return 1; }
        }
    SDL_free(rgba);
    assets_free(&ga);
    return 0;
}
