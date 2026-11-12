/*
 * The Ancient Art of War (1984) - remake
 * Nov 2026 using Claude Opus 5.5
 * "Guided" by Brett Hallen
 * Original game by Dave Murry & Barry Murry
 * Published by Brøderbund Software Inc.
 *
 * test/vgapics.c: a test or debugging aid for the remake (see README.md here)
 */
#include "../src/assets.h"
#include <SDL3/SDL.h>
static GameAssets ga;
int main(int argc, char **argv)
{
    (void)argc; char err[256];
    if (!assets_load(&ga, argv[1], err, sizeof err)) return 1;
    for (int k = PIC_TITLE; k <= PIC_TITLE_BARBARIAN; k++) {
        const Image *im = &ga.set[MODE_VGA].pictures[k];
        SDL_Surface *s = SDL_CreateSurface(im->w, im->h, SDL_PIXELFORMAT_XRGB8888);
        for (int y = 0; y < im->h; y++) for (int x = 0; x < im->w; x++) {
            RGB c = im->pal[im->px[y * im->w + x]];
            ((uint32_t *)s->pixels)[y * s->pitch / 4 + x] = (c.r << 16) | (c.g << 8) | c.b;
        }
        char p[256]; SDL_snprintf(p, sizeof p, "%s/vga_%d.bmp", argv[2], k); SDL_SaveBMP(s, p);
        SDL_Log("%d: %dx%d", k, im->w, im->h);
    }
}
