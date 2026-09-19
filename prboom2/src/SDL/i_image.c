#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __vita__

#include <string.h>

#include "SDL.h"
#include "SDL_image.h"

#include "i_image.h"

/* SDL_image keeps this format-specific loader private in its public header,
 * but exports it from the Vita SDK archive.  Using it for GIF avoids
 * IMG_LoadGIF_RW(), whose animation cleanup helper lives in IMG.c.obj and
 * would reintroduce the generic/WebP dispatcher. */
extern SDL_Surface *IMG_LoadSTB_RW(SDL_RWops *src);

static int I_ImageReadHeader(SDL_RWops *src, Sint64 start,
                             unsigned char *header, size_t size)
{
  if (SDL_RWseek(src, start, RW_SEEK_SET) < 0)
    return 0;

  return SDL_RWread(src, header, 1, size) == size;
}

static int I_ImageReset(SDL_RWops *src, Sint64 start)
{
  return SDL_RWseek(src, start, RW_SEEK_SET) >= 0;
}

SDL_Surface *I_LoadImageRW(SDL_RWops *src, int freesrc)
{
  unsigned char header[16];
  Sint64 start;
  SDL_Surface *surface = NULL;

  if (!src)
  {
    SDL_SetError("I_LoadImageRW: invalid SDL_RWops");
    return NULL;
  }

  start = SDL_RWtell(src);
  if (start < 0 || !I_ImageReadHeader(src, start, header, sizeof(header)))
  {
    SDL_SetError("I_LoadImageRW: could not read image header");
    goto done;
  }

  /* PNG: all image lumps shipped by the Vita port use this format. */
  if (!memcmp(header, "\x89PNG\r\n\x1a\n", 8))
  {
    if (I_ImageReset(src, start))
      surface = IMG_LoadPNG_RW(src);
    goto done;
  }

  /* JPEG, GIF and BMP are supported by PrBoom+'s high-resolution format set. */
  if (header[0] == 0xff && header[1] == 0xd8 && header[2] == 0xff)
  {
    if (I_ImageReset(src, start))
      surface = IMG_LoadJPG_RW(src);
    goto done;
  }

  if (!memcmp(header, "GIF87a", 6) || !memcmp(header, "GIF89a", 6))
  {
    if (I_ImageReset(src, start))
      surface = IMG_LoadSTB_RW(src);
    goto done;
  }

  if (!memcmp(header, "BM", 2))
  {
    if (I_ImageReset(src, start))
      surface = IMG_LoadBMP_RW(src);
    goto done;
  }

  /* PCX has no signature string, but has a stable manufacturer/version header. */
  if (header[0] == 0x0a && header[2] == 1 && header[1] <= 5)
  {
    if (I_ImageReset(src, start))
      surface = IMG_LoadPCX_RW(src);
    goto done;
  }

  /* WebP is intentionally not a supported Vita image format. */
  if (!memcmp(header, "RIFF", 4) && !memcmp(header + 8, "WEBP", 4))
  {
    SDL_SetError("WebP images are disabled on Vita");
    goto done;
  }

  /* TGA has no reliable magic; preserve the original fallback behavior. */
  if (I_ImageReset(src, start))
    surface = IMG_LoadTGA_RW(src);

done:
  if (freesrc)
    SDL_FreeRW(src);

  return surface;
}

SDL_Surface *I_LoadImageFile(const char *path)
{
  SDL_RWops *src = SDL_RWFromFile(path, "rb");

  if (!src)
    return NULL;

  return I_LoadImageRW(src, 1);
}

#endif /* __vita__ */
