#include <taihen.h>

/*
 * These definitions exist only so vita-elf-export can inspect an ELF and
 * generate the taiHEN import archive.  They are never packaged as a runtime
 * module and must not be mistaken for taiHEN's implementation.
 */
SceUID taiHookFunctionOffsetForUser(tai_hook_ref_t *p_hook, tai_offset_args_t *args)
{
    (void)p_hook;
    (void)args;
    return -1;
}

int taiGetModuleInfo(const char *module, tai_module_info_t *info)
{
    (void)module;
    (void)info;
    return -1;
}

int taiHookRelease(SceUID tai_uid, tai_hook_ref_t hook)
{
    (void)tai_uid;
    (void)hook;
    return -1;
}

SceUID taiInjectDataForUser(tai_offset_args_t *args)
{
    (void)args;
    return -1;
}

int taiInjectRelease(SceUID tai_uid)
{
    (void)tai_uid;
    return -1;
}

int main(void)
{
    return 0;
}
