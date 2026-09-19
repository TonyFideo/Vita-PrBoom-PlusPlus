#ifndef I_IMAGE_H
#define I_IMAGE_H

#ifdef __vita__

#include "SDL.h"

/*
 * SDL2_image's generic loaders pull every optional decoder from its static
 * archive.  On Vita that also pulls the WebP decoder, which is not used by
 * the port and is unsafe with the Vita SDK build.  These wrappers dispatch
 * only to the format-specific loaders that the original Vita port needs.
 */
SDL_Surface *I_LoadImageRW(SDL_RWops *src, int freesrc);
SDL_Surface *I_LoadImageFile(const char *path);

#endif /* __vita__ */

#endif /* I_IMAGE_H */
