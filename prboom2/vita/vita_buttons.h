/*
 * Vita system button-assignment helper.
 *
 * This describes menu semantics only.  Gameplay bindings continue to use
 * their physical/controller assignments and must not be remapped through
 * this helper.
 */

#ifndef PRBOOM_VITA_BUTTONS_H
#define PRBOOM_VITA_BUTTONS_H

/* Values defined by SceSystemParamEnterButtonAssign in VitaSDK. */
#define VITA_ENTER_BUTTON_CIRCLE 0
#define VITA_ENTER_BUTTON_CROSS  1

/* AppUtil must have been initialized by the caller before this function is
 * used.  The result is cached for the lifetime of the process. */
int VitaButtons_Initialize(void);
int VitaButtons_GetEnterButton(void);
int VitaButtons_IsCircleConfirm(void);
int VitaButtons_GetQueryResult(void);

#endif
