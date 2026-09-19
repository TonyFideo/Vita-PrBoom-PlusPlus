/*
 * Vita color saturation control.
 *
 * OpenGL saturation is implemented by the modern VitaGL fixed-function
 * fragment shader.  This file only transfers the menu value to VitaGL; it
 * deliberately does not read or rewrite the display buffer on the CPU.
 */

#include "gl_saturation.h"

#ifdef __vita__

#include "i_system.h"
#include "m_misc.h"
#include "gl_opengl.h"

static int saturation_initialized;

static float VitaSaturation_Factor(void)
{
  int value = vita_color_saturation;

  if (value < 0)
    value = 0;
  else if (value > 200)
    value = 200;

  return (float)value / 100.0f;
}

void VitaSaturation_Init(void)
{
  if (saturation_initialized)
    return;

  saturation_initialized = 1;
  vglSetColorSaturation(VitaSaturation_Factor());
  I_AtExit(VitaSaturation_Shutdown, true);
}

void VitaSaturation_BeginFrame(void)
{
  if (saturation_initialized)
    vglSetColorSaturation(VitaSaturation_Factor());
}

void VitaSaturation_EndFrame(void) {}

void VitaSaturation_Shutdown(void)
{
  saturation_initialized = 0;
}

#else

void VitaSaturation_Init(void) {}
void VitaSaturation_BeginFrame(void) {}
void VitaSaturation_EndFrame(void) {}
void VitaSaturation_Shutdown(void) {}

#endif
