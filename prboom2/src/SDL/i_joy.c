/* Emacs style mode select   -*- C++ -*-
 *-----------------------------------------------------------------------------
 * PrBoom joystick handling adapted for the Vita SDL game-controller backend.
 * Distributed under the GNU General Public License version 2.
 *-----------------------------------------------------------------------------
 */

#include <stdlib.h>

#include "SDL.h"
#include "doomdef.h"
#include "doomtype.h"
#include "m_argv.h"
#include "d_event.h"
#include "d_main.h"
#include "i_joy.h"
#include "i_system.h"
#include "lprintf.h"
#ifdef __vita__
#include "m_misc.h"
#endif

#define TRIGGER_DEADZONE 16384

int joyaxis_moveh;
int joyaxis_movev;
int joyaxis_lookh;
int joyaxis_lookv;

/* Launcher values use a 0-16 scale. */
int joy_deadzone_left = 1;
int joy_deadzone_right = 1;
int joy_permastrafe = 1;

int usejoystick;

static SDL_GameController *joystick;
static int prev_axis[SDL_CONTROLLER_AXIS_MAX];
#ifndef __vita__
static int real_deadzone_left;
static int real_deadzone_right;
#endif

static void I_EndJoystick(void)
{
  lprintf(LO_DEBUG, "I_EndJoystick : closing joystick\n");
  if (joystick)
  {
    SDL_GameControllerClose(joystick);
    joystick = NULL;
  }
}

#ifdef __vita__
static int VitaClampDeadzoneAmount(int amount)
{
  if (amount < 0)
    return 0;
  if (amount > 100)
    return 100;
  return amount;
}

static int VitaAxisDeadzone(const int axis)
{
  int amount;
  int enabled;

  if (axis < 2)
  {
    enabled = vita_left_stick_deadzone;
    amount = vita_left_stick_deadzone_amount;
  }
  else
  {
    enabled = vita_right_stick_deadzone;
    amount = vita_right_stick_deadzone_amount;
  }

  if (!enabled)
    return 0;

  amount = VitaClampDeadzoneAmount(amount);
  return (32767 * amount + 50) / 100;
}

/* Remove the configured center deadzone, then expand the remaining range back
 * to the full signed SDL axis range. This prevents the deadzone setting from
 * reducing the maximum movement or camera speed. */
static int VitaRemapAxis(const int value, const int deadzone)
{
  int magnitude;
  int available;

  if (abs(value) <= deadzone)
    return 0;

  available = 32767 - deadzone;
  if (available <= 0)
    return 0;

  magnitude = abs(value) - deadzone;
  magnitude = (magnitude * 32767) / available;

  return value < 0 ? -magnitude : magnitude;
}
#endif

static int GetAxis(const int axis)
{
  const int val = SDL_GameControllerGetAxis(joystick, axis);
#ifdef __vita__
  return VitaRemapAxis(val, VitaAxisDeadzone(axis));
#else
  const int dz = (axis < 2) ? real_deadzone_left : real_deadzone_right;
  return (abs(val) > dz) ? val : 0;
#endif
}

static int JoystickMove(const int axis)
{
#ifndef __vita__
  int axis_value;
#endif

  if (axis < 0 || axis >= SDL_CONTROLLER_AXIS_MAX)
    return 0;

  prev_axis[axis] = GetAxis(axis);
#ifdef __vita__
  if (!prev_axis[axis])
    return 0;

  if (vita_joystick_movement_mode)
  {
    int magnitude = (abs(prev_axis[axis]) * 100 + 16383) / 32767;

    if (magnitude > 100)
      magnitude = 100;
    return prev_axis[axis] < 0 ? -magnitude : magnitude;
  }

  /* Digital mode still honors the configured deadzone, but emits a clean
   * -1/0/+1 value so the same event can drive both gameplay and menus. */
  return prev_axis[axis] < 0 ? -1 : 1;
#else
  axis_value = prev_axis[axis] / 3000;
  return abs(axis_value) < 7 ? 0 : axis_value;
#endif
}

static int JoystickLook(const int axis)
{
  if (axis < 0 || axis >= SDL_CONTROLLER_AXIS_MAX)
    return 0;

  prev_axis[axis] = GetAxis(axis);
  return prev_axis[axis] >> 4;
}

void I_PollJoystick(void)
{
  event_t ev;
  int i;

  if (!usejoystick || !joystick)
    return;

#ifndef __vita__
  real_deadzone_left = 32768.0f * (float)joy_deadzone_left / 16.0f;
  real_deadzone_right = 32768.0f * (float)joy_deadzone_right / 16.0f;
#endif

  ev.type = ev_joystick;
  ev.data1 = 0;
  ev.data2 = JoystickMove(joyaxis_moveh);
  ev.data3 = JoystickMove(joyaxis_movev);
  D_PostEvent(&ev);

  /* 2.6.66 separates mouse buttons from mouse motion.  The right stick
     supplies camera motion, so it must use the motion event path. */
  ev.type = ev_mousemotion;
  ev.data1 = 0;
  ev.data2 = JoystickLook(joyaxis_lookh);
  ev.data3 = -JoystickLook(joyaxis_lookv);
  if (ev.data2 || ev.data3)
    D_PostEvent(&ev);

  /* Some SDL mappings expose triggers as axes. Preserve the original Vita
     translation so those mappings still generate key events. */
  ev.data2 = ev.data3 = 0;
  for (i = SDL_CONTROLLER_AXIS_TRIGGERLEFT;
       i <= SDL_CONTROLLER_AXIS_TRIGGERRIGHT; ++i)
  {
    const int axis_value = SDL_GameControllerGetAxis(joystick, i);
    ev.data1 = KEYD_JOY_BASE + i;
    if (axis_value >= TRIGGER_DEADZONE && prev_axis[i] < TRIGGER_DEADZONE)
    {
      ev.type = ev_keydown;
      D_PostEvent(&ev);
    }
    else if (axis_value < TRIGGER_DEADZONE && prev_axis[i] >= TRIGGER_DEADZONE)
    {
      ev.type = ev_keyup;
      D_PostEvent(&ev);
    }
    prev_axis[i] = axis_value;
  }
}

void I_InitJoystick(void)
{
  const char *fname = "I_InitJoystick : ";
  int num_joysticks;

  if (!usejoystick || M_CheckParm("-nojoy"))
    return;

  if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0)
  {
    lprintf(LO_ERROR, "%sSDL initialization failed: %s\n", fname,
            SDL_GetError());
    return;
  }

  num_joysticks = SDL_NumJoysticks();
  if (usejoystick > num_joysticks || usejoystick <= 0 ||
      !SDL_IsGameController(usejoystick - 1))
  {
    lprintf(LO_WARN, "%sinvalid joystick %d (found %d)\n", fname,
            usejoystick, num_joysticks);
    return;
  }

  joystick = SDL_GameControllerOpen(usejoystick - 1);
  if (!joystick)
  {
    lprintf(LO_ERROR, "%serror opening joystick %d: %s\n", fname,
            usejoystick, SDL_GetError());
    return;
  }

  I_AtExit(I_EndJoystick, true);
  SDL_GameControllerEventState(SDL_ENABLE);
  lprintf(LO_INFO, "%sopened %s\n", fname,
          SDL_GameControllerName(joystick));
}
