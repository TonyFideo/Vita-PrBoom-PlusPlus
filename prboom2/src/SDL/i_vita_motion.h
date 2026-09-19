/*
 * Vita motion input interface.
 */

#ifndef __I_VITA_MOTION_H__
#define __I_VITA_MOTION_H__

#ifdef __vita__
void I_InitVitaMotion(void);
void I_PollVitaMotion(void);
void I_ShutdownVitaMotion(void);
#endif

#endif
