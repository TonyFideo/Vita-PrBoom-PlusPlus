/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 *
 *  PrBoom+: Vita mpg123 MP3 music player.
 *
 *  This backend is intentionally separate from madplayer.c.  libmad exposes
 *  decoded frames and a fixed-point synth buffer, while mpg123 exposes a
 *  stream of PCM bytes.  Keeping the adapters separate avoids an ABI/API
 *  compatibility layer that would only obscure error handling.
 *
 *  The source data belongs to the WAD/resource owner.  mpg123 receives a
 *  copy through its feed interface and the stream is reopened from that same
 *  memory when a looping song reaches EOF.
 *
 *-----------------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "musicplayer.h"

#ifndef HAVE_LIBMPG123

/* This file is only added to the build when mpg123 is found.  Keep a small
 * fallback so an IDE or an unusual source-list consumer can still parse it. */
#include <string.h>

static const char *mpg123_name(void)
{
  return "mad mp3 player (DISABLED)";
}

static int mpg123_init_disabled(int samplerate)
{
  (void)samplerate;
  return 0;
}

const music_player_t mpg123_player =
{
  mpg123_name,
  mpg123_init_disabled,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
};

#else

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <mpg123.h>

#include "i_sound.h"
#include "lprintf.h"

/* mpg123 can return a complete MPEG frame at once.  The Vita audio callback
 * asks for short blocks, so this is deliberately bounded and reused between
 * calls instead of allocating in the render path. */
#define MPG123_READ_BUFFER_BYTES 16384
#define MPG123_PROBE_READS 16
#define MPG123_MAX_STALLS 8

static mpg123_handle *mpg_handle;
static int mp_library_ready;
static int mp_stream_open;

static int mp_looping;
static int mp_volume;
static int mp_samplerate_target;
static int mp_input_rate;
static int mp_input_channels;
static int mp_paused;
static int mp_playing;

static const void *mp_data;
static unsigned mp_len;

static short mp_pcm_buffer[MPG123_READ_BUFFER_BYTES / sizeof(short)];

static const char *mpg123_name(void)
{
  /* Keep the old logical name.  i_sound.c and existing profiles use it to
   * select this slot; the runtime log identifies the actual backend. */
  return "mad mp3 player";
}

static void mpg123_close_stream(void)
{
  if (mpg_handle && mp_stream_open)
    mpg123_close(mpg_handle);

  mp_stream_open = 0;
}

static int mpg123_read_format(void)
{
  long rate;
  int channels;
  int encoding;

  if (!mpg_handle || mpg123_getformat(mpg_handle, &rate, &channels, &encoding) != MPG123_OK)
  {
    lprintf(LO_WARN, "mpg123: unable to read stream format: %s\n",
            mpg_handle ? mpg123_strerror(mpg_handle) : "invalid handle");
    return 0;
  }

  if (rate <= 0 || rate > INT_MAX || (channels != MPG123_MONO && channels != MPG123_STEREO))
  {
    lprintf(LO_WARN, "mpg123: unsupported format rate=%ld channels=%d\n",
            rate, channels);
    return 0;
  }

  if (encoding != MPG123_ENC_SIGNED_16)
  {
    lprintf(LO_WARN, "mpg123: unsupported output encoding %d\n", encoding);
    return 0;
  }

  if (mp_input_rate && (mp_input_rate != (int)rate || mp_input_channels != channels))
  {
    lprintf(LO_WARN,
            "mpg123: stream format changed from %d Hz/%d ch to %ld Hz/%d ch\n",
            mp_input_rate, mp_input_channels, rate, channels);
  }

  mp_input_rate = (int)rate;
  mp_input_channels = channels;
  return 1;
}

static int mpg123_open_stream(const void *data, unsigned len)
{
  int result;

  if (!mpg_handle || !data || !len)
    return 0;

  mpg123_close_stream();

  result = mpg123_open_feed(mpg_handle);
  if (result != MPG123_OK)
  {
    lprintf(LO_WARN, "mpg123: open_feed failed: %s\n", mpg123_strerror(mpg_handle));
    return 0;
  }
  mp_stream_open = 1;

  result = mpg123_feed(mpg_handle, (const unsigned char *)data, (size_t)len);
  if (result != MPG123_OK)
  {
    lprintf(LO_WARN, "mpg123: feed failed: %s\n", mpg123_strerror(mpg_handle));
    mpg123_close_stream();
    return 0;
  }

  return 1;
}

static int mpg123_probe(const void *data, unsigned len)
{
  int result;
  int i;
  int got_audio = 0;
  int stalls = 0;
  size_t done;

  if (!mpg123_open_stream(data, len))
    return 0;

  /* getformat parses the first frame and also consumes the initial
   * MPG123_NEW_FORMAT notification for the normal case. */
  if (!mpg123_read_format())
  {
    mpg123_close_stream();
    return 0;
  }

  /* Decode a small amount during registration.  Merely accepting the input
   * into the feed buffer is not enough: malformed/non-MP3 WAD lumps could
   * otherwise be selected and fail later in the audio callback. */
  for (i = 0; i < MPG123_PROBE_READS && !got_audio; i++)
  {
    done = 0;
    result = mpg123_read(mpg_handle, mp_pcm_buffer, sizeof(mp_pcm_buffer), &done);

    if (done)
      got_audio = 1;

    if (result == MPG123_NEW_FORMAT)
    {
      if (!mpg123_read_format())
      {
        mpg123_close_stream();
        return 0;
      }
      continue;
    }

    if (result == MPG123_DONE || result == MPG123_NEED_MORE)
      break;

    if (result != MPG123_OK)
    {
      lprintf(LO_WARN, "mpg123: registration failed: %s\n",
              mpg123_strerror(mpg_handle));
      mpg123_close_stream();
      return 0;
    }

    if (!done)
    {
      stalls++;
      if (stalls >= MPG123_MAX_STALLS)
        break;
    }
    else
      stalls = 0;
  }

  mpg123_close_stream();

  if (!got_audio)
  {
    lprintf(LO_WARN, "mpg123: registration produced no PCM data\n");
    return 0;
  }

  return 1;
}

static int mpg123_init_player(int samplerate)
{
  int error = MPG123_OK;

  if (samplerate <= 0)
    return 0;

  if (mpg123_init() != MPG123_OK)
  {
    lprintf(LO_WARN, "mpg123: library initialization failed\n");
    return 0;
  }
  mp_library_ready = 1;

  mpg_handle = mpg123_new(NULL, &error);
  if (!mpg_handle)
  {
    lprintf(LO_WARN, "mpg123: handle creation failed (%d)\n", error);
    mpg123_exit();
    mp_library_ready = 0;
    return 0;
  }

  if (mpg123_format_none(mpg_handle) != MPG123_OK ||
      mpg123_format(mpg_handle, 0, MPG123_MONO | MPG123_STEREO,
                    MPG123_ENC_SIGNED_16) != MPG123_OK)
  {
    lprintf(LO_WARN, "mpg123: unable to configure signed 16-bit output: %s\n",
            mpg123_strerror(mpg_handle));
    mpg123_delete(mpg_handle);
    mpg_handle = NULL;
    mpg123_exit();
    mp_library_ready = 0;
    return 0;
  }

  mp_samplerate_target = samplerate;
  mp_input_rate = 0;
  mp_input_channels = 0;
  mp_volume = 0;
  mp_looping = 0;
  mp_paused = 0;
  mp_playing = 0;
  mp_data = NULL;
  mp_len = 0;
  return 1;
}

static void mpg123_shutdown(void)
{
  mpg123_close_stream();

  if (mpg_handle)
  {
    mpg123_delete(mpg_handle);
    mpg_handle = NULL;
  }

  if (mp_library_ready)
  {
    mpg123_exit();
    mp_library_ready = 0;
  }

  mp_playing = 0;
  mp_data = NULL;
  mp_len = 0;
}

static void mpg123_setvolume(int volume)
{
  if (volume < 0)
    volume = 0;
  if (volume > 15)
    volume = 15;
  mp_volume = volume;
}

static void mpg123_pause(void)
{
  mp_paused = 1;
}

static void mpg123_resume(void)
{
  mp_paused = 0;
}

static const void *mpg123_registersong(const void *data, unsigned len)
{
  if (!data || len < 4 || !mpg_handle)
    return NULL;

  /* mp_input_* is also updated by the probe and becomes the stable input
   * rate used by I_ResampleStream during playback. */
  if (!mpg123_probe(data, len))
    return NULL;

  mp_data = data;
  mp_len = len;
  lprintf(LO_INFO, "mpg123_registersong succeeded: %d Hz/%d ch\n",
          mp_input_rate, mp_input_channels);
  return data;
}

static void mpg123_unregistersong(const void *handle)
{
  (void)handle;
  mp_playing = 0;
  mpg123_close_stream();
  mp_data = NULL;
  mp_len = 0;
  mp_input_rate = 0;
  mp_input_channels = 0;
}

static int mpg123_start_song(void)
{
  int old_rate = mp_input_rate;
  int old_channels = mp_input_channels;

  if (!mpg123_open_stream(mp_data, mp_len) || !mpg123_read_format())
  {
    mpg123_close_stream();
    return 0;
  }

  /* A loop restart must preserve the format expected by the current
   * resampling callback.  The normal MP3 case is constant, but reject a
   * changed stream rather than silently producing incorrectly timed audio. */
  if (old_rate && (mp_input_rate != old_rate || mp_input_channels != old_channels))
  {
    lprintf(LO_WARN, "mpg123: loop stream format is not stable\n");
    mpg123_close_stream();
    mp_input_rate = old_rate;
    mp_input_channels = old_channels;
    return 0;
  }

  return 1;
}

static void mpg123_play(const void *handle, int looping)
{
  (void)handle;

  mp_playing = 0;
  mp_looping = looping != 0;
  mp_paused = 0;

  if (!mp_data || !mp_len || !mpg123_start_song())
  {
    lprintf(LO_WARN, "mpg123: unable to start song\n");
    return;
  }

  mp_playing = 1;
}

static void mpg123_stop(void)
{
  mp_playing = 0;
  mpg123_close_stream();
}

static short mpg123_scale_sample(short sample)
{
  int value = ((int)sample * mp_volume) / 15;

  if (value < SHRT_MIN)
    value = SHRT_MIN;
  if (value > SHRT_MAX)
    value = SHRT_MAX;
  return (short)value;
}

static int mpg123_restart_loop(void)
{
  return mpg123_start_song();
}

static void mpg123_render_ex(void *dest, unsigned nsamp)
{
  short *output = (short *)dest;
  unsigned produced = 0;
  int stalls = 0;

  if (!mp_playing || mp_paused || !mpg_handle || !mp_stream_open ||
      (mp_input_channels != MPG123_MONO && mp_input_channels != MPG123_STEREO))
  {
    memset(dest, 0, (size_t)nsamp * 4);
    return;
  }

  while (produced < nsamp && mp_playing)
  {
    size_t capacity = (size_t)(nsamp - produced) * 4;
    size_t done = 0;
    unsigned bytes_per_frame = (unsigned)mp_input_channels * sizeof(short);
    int result;
    unsigned frames;
    unsigned i;

    if (capacity > sizeof(mp_pcm_buffer))
      capacity = sizeof(mp_pcm_buffer);

    result = mpg123_read(mpg_handle, mp_pcm_buffer, capacity, &done);

    if (result == MPG123_NEW_FORMAT)
    {
      if (!mpg123_read_format())
      {
        mp_playing = 0;
        break;
      }
      bytes_per_frame = (unsigned)mp_input_channels * sizeof(short);
    }

    if (done)
    {
      if (!bytes_per_frame || done % bytes_per_frame)
      {
        lprintf(LO_WARN, "mpg123: decoder returned an incomplete PCM frame\n");
        mp_playing = 0;
        break;
      }

      frames = (unsigned)(done / bytes_per_frame);
      if (frames > nsamp - produced)
        frames = nsamp - produced;

      for (i = 0; i < frames; i++)
      {
        short left = mp_pcm_buffer[i * mp_input_channels];
        short right = (mp_input_channels == MPG123_STEREO) ?
                      mp_pcm_buffer[i * mp_input_channels + 1] : left;
        output[(produced + i) * 2] = mpg123_scale_sample(left);
        output[(produced + i) * 2 + 1] = mpg123_scale_sample(right);
      }

      produced += frames;
      stalls = 0;
    }

    if (result == MPG123_DONE || result == MPG123_NEED_MORE)
    {
      if (produced < nsamp)
      {
        if (!mp_looping || !mpg123_restart_loop())
          mp_playing = 0;
      }
      continue;
    }

    if (result != MPG123_OK && result != MPG123_NEW_FORMAT)
    {
      lprintf(LO_WARN, "mpg123: decode failed: %s\n", mpg123_strerror(mpg_handle));
      mp_playing = 0;
      break;
    }

    if (!done)
    {
      stalls++;
      if (stalls >= MPG123_MAX_STALLS)
      {
        lprintf(LO_WARN, "mpg123: decoder made no progress\n");
        mp_playing = 0;
        break;
      }
    }
  }

  if (produced < nsamp)
    memset(output + produced * 2, 0, (size_t)(nsamp - produced) * 4);
}

static void mpg123_render(void *dest, unsigned nsamp)
{
  if (!mp_input_rate || !mp_samplerate_target)
  {
    memset(dest, 0, (size_t)nsamp * 4);
    return;
  }

  I_ResampleStream(dest, nsamp, mpg123_render_ex,
                   (unsigned)mp_input_rate,
                   (unsigned)mp_samplerate_target);
}

const music_player_t mpg123_player =
{
  mpg123_name,
  mpg123_init_player,
  mpg123_shutdown,
  mpg123_setvolume,
  mpg123_pause,
  mpg123_resume,
  mpg123_registersong,
  mpg123_unregistersong,
  mpg123_play,
  mpg123_stop,
  mpg123_render
};

#endif /* HAVE_LIBMPG123 */
