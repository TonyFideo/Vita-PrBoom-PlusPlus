/*
 * Vita on-screen keyboard support for the PrBoom game executable.
 *
 * The launcher has its own IME dialog because it is a separate executable.
 * This small service keeps the game-side dialog state independent while
 * letting the existing PrBoom menu continue to validate and apply values.
 */

#include "vita_ime.h"

#include <vitasdk.h>
#include <stdint.h>
#include <string.h>

#include "lprintf.h"
#include "vita_buttons.h"

#define VITA_IME_TITLE_LENGTH  127
#define VITA_IME_TEXT_LENGTH   2047

static uint16_t vita_ime_title[SCE_IME_DIALOG_MAX_TITLE_LENGTH];
static uint16_t vita_ime_initial[SCE_IME_DIALOG_MAX_TEXT_LENGTH];
static uint16_t vita_ime_input[SCE_IME_DIALOG_MAX_TEXT_LENGTH + 1];
static int vita_ime_initialized;
static int vita_ime_active;

static void ascii_to_utf16(uint16_t *dst, int dst_size, const char *src)
{
  int i = 0;

  if (!dst || dst_size <= 0)
    return;

  if (src)
    while (src[i] && i + 1 < dst_size)
    {
      dst[i] = (uint16_t)(unsigned char)src[i];
      i++;
    }

  dst[i] = 0;
}

static void utf16_to_ascii(char *dst, int dst_size, const uint16_t *src)
{
  int i = 0;
  int out = 0;

  if (!dst || dst_size <= 0)
    return;

  if (src)
    while (src[i] && out + 1 < dst_size)
    {
      uint16_t ch = src[i++];

      /* PrBoom's menu strings are byte-based. Keep ASCII input compact and
         discard unsupported Unicode characters without leaving gaps. */
      if (ch >= 0x80)
        continue;
      dst[out++] = (char)ch;
    }

  dst[out] = 0;
}

int VitaIme_Init(void)
{
  SceAppUtilInitParam app_util_init;
  SceAppUtilBootParam app_util_boot;
  SceCommonDialogConfigParam config;
  int value;
  int rc;

  if (vita_ime_initialized)
    return 0;

  /* SceIme depends on AppUtil/CommonDialog being initialized by the game
   * process. The launcher is a separate executable and initializing it there
   * does not carry over when the game is started. */
  rc = sceSysmoduleLoadModule(SCE_SYSMODULE_APPUTIL);
  if (rc < 0 && rc != SCE_SYSMODULE_LOADED)
  {
    lprintf(LO_ERROR, "VitaIme_Init: loading AppUtil failed: 0x%08X\n",
            (unsigned int)rc);
    return rc;
  }

  rc = sceSysmoduleLoadModule(SCE_SYSMODULE_IME);
  if (rc < 0 && rc != SCE_SYSMODULE_LOADED)
  {
    lprintf(LO_ERROR, "VitaIme_Init: loading IME failed: 0x%08X\n",
            (unsigned int)rc);
    return rc;
  }

  memset(&app_util_init, 0, sizeof(app_util_init));
  memset(&app_util_boot, 0, sizeof(app_util_boot));
  rc = sceAppUtilInit(&app_util_init, &app_util_boot);
  if (rc < 0)
  {
    lprintf(LO_ERROR, "VitaIme_Init: sceAppUtilInit failed: 0x%08X\n",
            (unsigned int)rc);
    return rc;
  }

  sceCommonDialogConfigParamInit(&config);

  /* sceCommonDialogConfigParamInit() uses MAX_VALUE sentinels for these two
   * fields. They are useful when the caller fills them from system settings,
   * but they are not valid values to submit if a getter is unavailable (for
   * example in Vita3K). Keep a valid fallback before attempting the getters. */
  config.language = SCE_SYSTEM_PARAM_LANG_ENGLISH_US;
  config.enterButtonAssign = SCE_SYSTEM_PARAM_ENTER_BUTTON_CROSS;

  /* Use the console's actual language and Cross/Circle assignment. The
   * previous zero/default configuration could leave the game dialog with a
   * different button assignment than the launcher. If Vita3K does not
   * implement these getters, the SDK defaults remain valid. */
  if (sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &value) >= 0)
    config.language = (SceSystemParamLang)value;
  VitaButtons_Initialize();
  config.enterButtonAssign = VitaButtons_GetEnterButton();

  rc = sceCommonDialogSetConfigParam(&config);
  if (rc < 0)
  {
    lprintf(LO_ERROR,
            "VitaIme_Init: sceCommonDialogSetConfigParam failed: 0x%08X\n",
            (unsigned int)rc);
    return rc;
  }

  vita_ime_initialized = 1;
  lprintf(LO_INFO, "VitaIme_Init: AppUtil/CommonDialog/IME ready\n");
  return 0;
}

void VitaIme_Shutdown(void)
{
  if (!vita_ime_initialized)
    return;

  if (vita_ime_active)
  {
    sceImeDialogAbort();
    sceImeDialogTerm();
    vita_ime_active = 0;
  }
}

int VitaIme_IsActive(void)
{
  return vita_ime_active;
}

int VitaIme_ShouldUpdateCommonDialog(void)
{
  /*
   * Vita's Common Dialog is serviced as part of the display-frame
   * submission, not only while sceImeDialogGetStatus() reports an active
   * IME.  In particular, sceImeDialogTerm() starts a close transition that
   * can span more than one submitted frame.  The original Vita launcher
   * keeps calling its Common Dialog update every frame, so do the same here
   * and let VitaGL skip the call internally when Common Dialog support is
   * unavailable.  Before the first keyboard is opened, AppUtil/CommonDialog
   * has not been initialized by this executable yet, so avoid submitting
   * updates during that earlier phase.
   */
  return vita_ime_initialized;
}

int VitaIme_Start(const char *title, const char *initial_text,
                  unsigned int type, int max_text_length)
{
  SceImeDialogParam param;
  int rc;

  if (vita_ime_active)
    return 0;

  if (!vita_ime_initialized && VitaIme_Init() < 0)
  {
    lprintf(LO_ERROR, "VitaIme_Start: IME services are unavailable\n");
    return 0;
  }

  if (max_text_length < 1)
    max_text_length = 1;
  if (max_text_length > VITA_IME_TEXT_LENGTH)
    max_text_length = VITA_IME_TEXT_LENGTH;

  memset(vita_ime_title, 0, sizeof(vita_ime_title));
  memset(vita_ime_initial, 0, sizeof(vita_ime_initial));
  memset(vita_ime_input, 0, sizeof(vita_ime_input));

  ascii_to_utf16(vita_ime_title, SCE_IME_DIALOG_MAX_TITLE_LENGTH,
                 title ? title : "Input value");
  ascii_to_utf16(vita_ime_initial, SCE_IME_DIALOG_MAX_TEXT_LENGTH,
                 initial_text);

  sceImeDialogParamInit(&param);
  param.supportedLanguages = 0x0001FFFF;
  param.languagesForced = SCE_TRUE;
  param.type = type;
  param.title = vita_ime_title;
  param.maxTextLength = (SceUInt32)max_text_length;
  param.initialText = vita_ime_initial;
  param.inputTextBuffer = vita_ime_input;

  rc = sceImeDialogInit(&param);
  if (rc < 0)
  {
    lprintf(LO_ERROR, "VitaIme_Start: sceImeDialogInit failed: 0x%08X\n",
            (unsigned int)rc);
    return 0;
  }

  vita_ime_active = 1;
  lprintf(LO_INFO, "VitaIme_Start: opened type=%u max=%d title=\"%s\"\n",
          type, max_text_length, title ? title : "Input value");
  return 1;
}

int VitaIme_Poll(char *output, int output_size)
{
  SceImeDialogResult result;
  SceCommonDialogStatus status;

  if (!vita_ime_active)
    return VITA_IME_RESULT_ERROR;

  status = sceImeDialogGetStatus();
  if ((int)status < 0)
  {
    lprintf(LO_ERROR, "VitaIme_Poll: sceImeDialogGetStatus failed: 0x%08X\n",
            (unsigned int)status);
    sceImeDialogAbort();
    sceImeDialogTerm();
    vita_ime_active = 0;
    return VITA_IME_RESULT_ERROR;
  }

  if (status != SCE_COMMON_DIALOG_STATUS_FINISHED)
    return VITA_IME_RESULT_PENDING;

  memset(&result, 0, sizeof(result));
  if (sceImeDialogGetResult(&result) < 0)
  {
    lprintf(LO_ERROR, "VitaIme_Poll: sceImeDialogGetResult failed\n");
    sceImeDialogTerm();
    vita_ime_active = 0;
    return VITA_IME_RESULT_ERROR;
  }

  if (output && output_size > 0)
    output[0] = 0;

  if (result.button == SCE_IME_DIALOG_BUTTON_ENTER &&
      output && output_size > 0)
    utf16_to_ascii(output, output_size, vita_ime_input);

  if (sceImeDialogTerm() < 0)
    lprintf(LO_WARN, "VitaIme_Poll: sceImeDialogTerm failed\n");
  vita_ime_active = 0;

  return result.button == SCE_IME_DIALOG_BUTTON_ENTER
    ? VITA_IME_RESULT_ACCEPTED
    : VITA_IME_RESULT_CANCELLED;
}
