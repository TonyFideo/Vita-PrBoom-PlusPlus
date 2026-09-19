/*
 * Vita on-screen keyboard support for the PrBoom game executable.
 */

#ifndef PRBOOM_VITA_IME_H
#define PRBOOM_VITA_IME_H

#define VITA_IME_RESULT_PENDING   1
#define VITA_IME_RESULT_ACCEPTED  0
#define VITA_IME_RESULT_CANCELLED (-1)
#define VITA_IME_RESULT_ERROR     (-2)

int VitaIme_Init(void);
void VitaIme_Shutdown(void);
int VitaIme_IsActive(void);
/* Once the game initializes the IME services, VitaGL must service Common
 * Dialog on every submitted display frame so the IME close transition can
 * finish after sceImeDialogTerm(). */
int VitaIme_ShouldUpdateCommonDialog(void);
int VitaIme_Start(const char *title, const char *initial_text,
                  unsigned int type, int max_text_length);
int VitaIme_Poll(char *output, int output_size);

#endif
