/*
 * Read the Vita system preference for the logical Enter/confirm button.
 *
 * SCE_CTRL_CROSS and SCE_CTRL_CIRCLE identify physical buttons.  They do not
 * tell an application which one the user selected as the system's Enter
 * button, so menus must use SceAppUtil's system-parameter getter instead.
 */

#include "vita_buttons.h"

#include <vitasdk.h>

static int vita_buttons_initialized;
static int vita_enter_button = VITA_ENTER_BUTTON_CROSS;
static int vita_buttons_query_result;

int VitaButtons_Initialize(void)
{
  int value = VITA_ENTER_BUTTON_CROSS;

  if (vita_buttons_initialized)
    return 0;

  vita_buttons_query_result = sceAppUtilSystemParamGetInt(
      SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &value);

  if (vita_buttons_query_result >= 0 &&
      (value == VITA_ENTER_BUTTON_CIRCLE ||
       value == VITA_ENTER_BUTTON_CROSS))
  {
    vita_enter_button = value;
  }
  else
  {
    /* Keep Cross as a safe fallback for Vita3K/older environments where the
     * getter is unavailable or returns an invalid value. */
    vita_enter_button = VITA_ENTER_BUTTON_CROSS;
  }

  vita_buttons_initialized = 1;
  return 0;
}

int VitaButtons_GetEnterButton(void)
{
  VitaButtons_Initialize();
  return vita_enter_button;
}

int VitaButtons_IsCircleConfirm(void)
{
  return VitaButtons_GetEnterButton() == VITA_ENTER_BUTTON_CIRCLE;
}

int VitaButtons_GetQueryResult(void)
{
  VitaButtons_Initialize();
  return vita_buttons_query_result;
}
