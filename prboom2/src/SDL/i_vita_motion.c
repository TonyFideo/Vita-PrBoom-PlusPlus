/*
 * Vita motion input.
 *
 * The first implementation exposes only horizontal gyro aim. It uses the
 * angular velocity reported by SceMotion, rather than mapping the device
 * orientation directly to a joystick axis. The result is a small camera
 * correction while the Vita is being rotated, with no mouse acceleration.
 */

#ifdef __vita__

#include <math.h>
#include <stdint.h>

#include <psp2/motion.h>

#include "doomstat.h"
#include "d_event.h"
#include "d_main.h"
#include "lprintf.h"
#include "m_misc.h"

#include "i_vita_motion.h"

#define VITA_GYRO_FILTER_ALPHA   0.35f
#define VITA_GYRO_MAX_DT         0.100f
#define VITA_DOOM_ANGLE_PER_RAD  10430.378350470453f
#define VITA_GYRO_SENSITIVITY_BOOST 1.50f /* global +50% response */

/* SceMotion uses the device's local axes. The Y angular velocity is the
 * rotation around the vertical screen axis and is used for horizontal aim.
 * The sign matches the existing right-stick camera direction. */
#define VITA_GYRO_YAW_SIGN       -1.0f

static dboolean vita_motion_sampling;
static uint64_t vita_motion_timestamp;
static float vita_gyro_yaw_rate;
static float vita_gyro_yaw_remainder;

static void VitaMotionResetFilter(void)
{
  vita_motion_timestamp = 0;
  vita_gyro_yaw_rate = 0.0f;
  vita_gyro_yaw_remainder = 0.0f;
}

void I_InitVitaMotion(void)
{
  int result;

  if (vita_motion_sampling)
    return;

  result = sceMotionStartSampling();
  if (result < 0 && result != SCE_MOTION_ERROR_ALREADY_SAMPLING)
  {
    lprintf(LO_WARN, "I_InitVitaMotion: sceMotionStartSampling failed: 0x%08x\n",
            result);
    return;
  }

  /* Keep the SDK's orientation corrections explicit. The first phase uses
   * angular velocity, so no orientation origin/reset is required. */
  sceMotionSetTiltCorrection(1);
  sceMotionSetGyroBiasCorrection(1);

  vita_motion_sampling = true;
  VitaMotionResetFilter();
  lprintf(LO_INFO, "I_InitVitaMotion: horizontal gyro ready\n");
}

void I_PollVitaMotion(void)
{
  SceMotionState state;
  uint64_t timestamp;
  float delta_time;
  float sensitivity;
  float turn;
  int turn_integer;
  event_t event;

  if (!vita_motion_sampling || !vita_gyro_aim ||
      gamestate != GS_LEVEL || menuactive || demoplayback)
  {
    VitaMotionResetFilter();
    return;
  }

  if (sceMotionGetState(&state) < 0)
  {
    VitaMotionResetFilter();
    return;
  }

  timestamp = (uint64_t)state.hostTimestamp;
  if (!timestamp)
    timestamp = (uint64_t)state.timestamp;

  if (!vita_motion_timestamp)
  {
    vita_motion_timestamp = timestamp;
    return;
  }

  if (timestamp <= vita_motion_timestamp)
  {
    VitaMotionResetFilter();
    return;
  }

  delta_time = (float)(timestamp - vita_motion_timestamp) / 1000000.0f;
  vita_motion_timestamp = timestamp;

  /* Ignore a long pause/frame stall instead of turning it into a large jump. */
  if (delta_time <= 0.0f || delta_time > VITA_GYRO_MAX_DT)
  {
    vita_gyro_yaw_rate = 0.0f;
    vita_gyro_yaw_remainder = 0.0f;
    return;
  }

  /* Keep the complete sensor signal. Small rotations must be able to move
   * the camera; the low-pass filter smooths the response without imposing a
   * minimum movement threshold. */
  vita_gyro_yaw_rate +=
    (state.angularVelocity.y - vita_gyro_yaw_rate) * VITA_GYRO_FILTER_ALPHA;

  /* 50 is the neutral value: it applies a 1.0x multiplier. */
  sensitivity = (float)vita_horizontal_sensitivity / 50.0f;
  turn = VITA_GYRO_YAW_SIGN * vita_gyro_yaw_rate * delta_time *
         VITA_DOOM_ANGLE_PER_RAD * sensitivity;
  turn *= VITA_GYRO_SENSITIVITY_BOOST;
  vita_gyro_yaw_remainder += turn;

  turn_integer = (int)vita_gyro_yaw_remainder;
  vita_gyro_yaw_remainder -= (float)turn_integer;
  if (!turn_integer)
    return;

  event.type = ev_gyro;
  event.data1 = 0;
  event.data2 = turn_integer;
  event.data3 = 0;
  D_PostEvent(&event);
}

void I_ShutdownVitaMotion(void)
{
  if (!vita_motion_sampling)
    return;

  sceMotionStopSampling();
  vita_motion_sampling = false;
  VitaMotionResetFilter();
}

#endif /* __vita__ */
