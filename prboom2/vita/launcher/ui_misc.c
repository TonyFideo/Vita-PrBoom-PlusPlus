#include "utils.h"
#include "files.h"
#include "input.h"
#include "screen.h"
#include "ui.h"

void UI_MenuMisc_Init(void);
void UI_MenuMisc_Update(void);
void UI_MenuMisc_Draw(void);
void UI_MenuMisc_Reload(void);

static const char *mon_labels[] = 
{
    "Default",
    "Disabled",
    "Fast",
    "Respawning",
    "Fast and Respawning",
};

static const char *mon_values[] = 
{
    "0", "1", "2", "4", "6",
};

static const char *skill_labels[] =
{
    "Default",
    "I'm too young to die",
    "Hey, not too rough",
    "Hurt me plenty",
    "Ultra Violence",
    "Nightmare!"
};

static const char *skill_values[] =
{
    "0", "1", "2", "3", "4", "5"
};

static const char *compatibility_labels[] =
{
    "-1 - Default (Latest PrBoom)",
    "0 - Doom v1.2",
    "1 - Doom v1.666",
    "2 - Doom/Doom II v1.9",
    "3 - Ultimate Doom/Doom95",
    "4 - Final Doom",
    "5 - DosDoom 0.47",
    "6 - TASDoom",
    "7 - Boom Compatibility",
    "8 - Boom v2.01",
    "9 - Boom v2.02",
    "10 - LxDoom v1.3.2+",
    "11 - MBF",
    "12 - PrBoom 2.03beta",
    "13 - PrBoom 2.1.0-2.1.1",
    "14 - PrBoom 2.2.x",
    "15 - PrBoom 2.3.x",
    "16 - PrBoom 2.4.0",
    "17 - Latest PrBoom",
};

static const char *compatibility_values[] =
{
    "-1", "0", "1", "2", "3", "4", "5", "6", "7", "8",
    "9", "10", "11", "12", "13", "14", "15", "16", "17",
};

static struct Option misc_opts[] =
{
    { OPT_CHOICE, "Monsters", .choice = { mon_labels, mon_values, 5, 0 } },
    { OPT_BOOLEAN, "Record demo" },
    { OPT_CHOICE, "Skill", .choice = { skill_labels, skill_values, 6, 0 } },
    { OPT_INTEGER, "Starting map", .inum = { 0, 99, 1, 0 } },
    { OPT_INT_CHOICE, "Compatibility level", .choice = { compatibility_labels, compatibility_values, 19, 0 } },
    { OPT_BOOLEAN, "EXPERIMENTAL fast sight calculations", .cfgvar = "checksight12" },
    { OPT_BOOLEAN, "Debug logging" },
#ifdef HAVE_PROFILING
    { OPT_BOOLEAN, "Profiling" },
#endif
    { OPT_BOOLEAN, "Advanced logging" },
    { OPT_BOOLEAN, "Render/VitaGL logging" },
};

struct Menu ui_menu_misc =
{
    MENU_PWADS,
    "Gameplay/Misc",
    "Gameplay/Misc",
    NULL, 0, 0, 0,
    UI_MenuMisc_Init,
    UI_MenuMisc_Update,
    UI_MenuMisc_Draw,
    UI_MenuMisc_Reload,
};

static struct Menu *self = &ui_menu_misc;

void UI_MenuMisc_Init(void)
{
    UI_MenuMisc_Reload();
}

void UI_MenuMisc_Update(void)
{
}

void UI_MenuMisc_Draw(void)
{

}

void UI_MenuMisc_Reload(void)
{
    misc_opts[0].codevar = fs_profiles[ui_profile].monsters;
    misc_opts[1].codevar = &fs_profiles[ui_profile].record;
    misc_opts[2].codevar = &fs_profiles[ui_profile].skill;
    misc_opts[3].codevar = &fs_profiles[ui_profile].warp;
    misc_opts[4].codevar = &fs_profiles[ui_profile].complevel;
#ifdef HAVE_PROFILING
    misc_opts[6].codevar = &fs_profiles[ui_profile].logfile;
    misc_opts[7].codevar = &fs_profiles[ui_profile].log_profiling;
    misc_opts[8].codevar = &fs_profiles[ui_profile].log_advanced;
    misc_opts[9].codevar = &fs_profiles[ui_profile].log_render;
#else
    misc_opts[6].codevar = &fs_profiles[ui_profile].logfile;
    misc_opts[7].codevar = &fs_profiles[ui_profile].log_advanced;
    misc_opts[8].codevar = &fs_profiles[ui_profile].log_render;
#endif

    self->opts = misc_opts;
    self->numopts = sizeof(misc_opts) / sizeof(*misc_opts);
}
