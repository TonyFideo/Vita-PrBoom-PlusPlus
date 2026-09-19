#pragma once

/* Runtime logging categories shared by the Vita executable and VitaGL. */
enum VitaLogCategory
{
    VITA_LOG_BASIC = 1u << 0,
    VITA_LOG_ADVANCED = 1u << 1,
    VITA_LOG_PROFILING = 1u << 2,
    VITA_LOG_RENDER = 1u << 3,
};

#ifdef __cplusplus
extern "C" {
#endif

/* Refresh the flags after response files have been expanded. */
void VitaLog_Refresh(void);
unsigned int VitaLog_GetFlags(void);
int VitaLog_IsEnabled(unsigned int category);

#ifdef __cplusplus
}
#endif
