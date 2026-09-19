/*
 * Vita-only GPU saturation control used by the Vita Features setting.  The
 * public functions are no-ops outside the Vita GL target.
 */

#ifndef PRBOOM_GL_SATURATION_H
#define PRBOOM_GL_SATURATION_H

void VitaSaturation_Init(void);
void VitaSaturation_BeginFrame(void);
void VitaSaturation_EndFrame(void);
void VitaSaturation_Shutdown(void);

#endif
