/*
 * Compatibility decoder for DUMB's FMOD/Ogg-Vorbis XM sample extension.
 *
 * DUMB's XM reader references dumb_decode_vorbis(), but the original DUMB
 * distribution leaves that callback to the host application.  PrBoom++
 * already links libvorbisfile for its Ogg music player, so use the same
 * library here instead of importing ZMusic's C++ decoder layer.
 */

#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef PRBOOM_DUMB_HAVE_VORBIS

#include <vorbis/vorbisfile.h>

typedef struct dumb_vorbis_memory_s
{
  const unsigned char *data;
  size_t length;
  size_t position;
} dumb_vorbis_memory_t;

static size_t dumb_vorbis_read(void *destination, size_t size,
                               size_t count, void *source)
{
  dumb_vorbis_memory_t *memory = (dumb_vorbis_memory_t *)source;
  size_t available;
  size_t bytes;

  if (!size || !count || memory->position >= memory->length)
    return 0;

  available = memory->length - memory->position;
  if (count > available / size)
    count = available / size;

  bytes = size * count;
  memcpy(destination, memory->data + memory->position, bytes);
  memory->position += bytes;
  return count;
}

static int dumb_vorbis_seek(void *source, ogg_int64_t offset, int whence)
{
  dumb_vorbis_memory_t *memory = (dumb_vorbis_memory_t *)source;
  ogg_int64_t target;

  switch (whence)
  {
    case SEEK_SET:
      target = offset;
      break;
    case SEEK_CUR:
      target = (ogg_int64_t)memory->position + offset;
      break;
    case SEEK_END:
      target = (ogg_int64_t)memory->length + offset;
      break;
    default:
      return -1;
  }

  if (target < 0 || (uint64_t)target > (uint64_t)memory->length)
    return -1;

  memory->position = (size_t)target;
  return 0;
}

static int dumb_vorbis_close(void *source)
{
  (void)source;
  return 0;
}

static long dumb_vorbis_tell(void *source)
{
  dumb_vorbis_memory_t *memory = (dumb_vorbis_memory_t *)source;
  if (memory->position > (size_t)LONG_MAX)
    return -1;
  return (long)memory->position;
}

#endif

short *dumb_decode_vorbis(int outlen, const void *oggstream, int sizebytes)
{
  short *samples;

  if (outlen <= 0)
    return NULL;

  samples = (short *)calloc(1, (size_t)outlen);
  if (!samples || !oggstream || sizebytes <= 0)
    return samples;

#ifdef PRBOOM_DUMB_HAVE_VORBIS
  {
    dumb_vorbis_memory_t memory;
    OggVorbis_File vorbis;
    ov_callbacks callbacks;
    vorbis_info *info;
    size_t wanted;
    size_t written = 0;
    int bitstream;

    memory.data = (const unsigned char *)oggstream;
    memory.length = (size_t)sizebytes;
    memory.position = 0;

    callbacks.read_func = dumb_vorbis_read;
    callbacks.seek_func = dumb_vorbis_seek;
    callbacks.close_func = dumb_vorbis_close;
    callbacks.tell_func = dumb_vorbis_tell;

    if (ov_open_callbacks(&memory, &vorbis, NULL, 0, callbacks) != 0)
      return samples;

    info = ov_info(&vorbis, -1);
    if (!info || info->channels != 1)
    {
      ov_clear(&vorbis);
      return samples;
    }

    wanted = (size_t)outlen / sizeof(*samples);
    while (written < wanted)
    {
      long bytes;
      size_t request = (wanted - written) * sizeof(*samples);
      if (request > 4096)
        request = 4096;

      bytes = ov_read(&vorbis, (char *)samples + written * sizeof(*samples),
                      (int)request, 0, 2, 1, &bitstream);
      if (bytes <= 0)
        break;
      written += (size_t)bytes / sizeof(*samples);
    }

    ov_clear(&vorbis);
  }
#else
  (void)oggstream;
  (void)sizebytes;
#endif

  return samples;
}
