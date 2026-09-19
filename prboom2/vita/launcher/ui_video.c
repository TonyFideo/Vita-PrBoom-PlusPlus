#include "utils.h"
#include "files.h"
#include "input.h"
#include "screen.h"
#include "configs.h"
#include "ui.h"

void UI_MenuVideoAudio_Init(void);
void UI_MenuVideoAudio_Update(void);
void UI_MenuVideoAudio_Draw(void);
void UI_MenuVideoAudio_Reload(void);

static const char *vidmode_labels[] =
{
    "Software 8-bit", "Software 16-bit", "Software 32-bit",
    "VitaGL",
};

/* The engine still expects the legacy configuration value "OpenGL". */
static const char *vidmode_values[] =
{
    "8-bit", "16-bit", "32-bit",
    "OpenGL",
};

static const char *resolutions[] =
{
    "320x200", "320x240", "480x272", "640x368",
    "640x400", "640x480", "720x408", "960x544"
};

static const char *scale_labels_software[] =
{
    "Keep aspect", "Integer scaling", "Fit to screen",
    "None", "2x"
};
static const char *scale_values_software[] = { "-2", "-1", "0", "1", "2" };
static const char *scale_labels_vitagl[] =
{
    "Keep aspect", "Integer scaling", "Fit to screen", "None"
};
static const char *scale_values_vitagl[] = { "-2", "-1", "0", "1" };

static const char *scale_filter_labels[] =
{
    "Nearest", "Linear"
};
static const char *scale_filter_values[] = { "0", "1" };

static const char *glsky_labels[] = { "Auto", "None", "Flat", "Dome" };
static const char *glsky_values[] = { "0", "1", "4", "3" };

static const char *multisampling[] = { "0", "2", "4" };

static const char *sample_rates[] = { "11025", "22050", "44100", "48000" };

static const char *music_type_labels[] = { "OPL2", "FluidSynth" };
static const char *music_type_values[] = { "opl2", "fluidsynth" };

static const char *sound_type_labels[] = { "Digital", "PC Speaker" };
static const char *sound_type_values[] = { "0", "1" };

enum VideoAudioOption
{
    VIDEO_SECTION,
    VIDEO_RENDER_MODE,
    VIDEO_RESOLUTION,
    VIDEO_SCALING_FILTER,
    VIDEO_SCALING,
    VIDEO_MSAA,
    VIDEO_SKY_MODE,
    VIDEO_AUDIO_SEPARATOR,
    AUDIO_SECTION,
    AUDIO_SOUND_TYPE,
    AUDIO_MIDI_PLAYER,
    AUDIO_SOUNDFONT,
    AUDIO_SAMPLE_RATE,
    AUDIO_CHANNELS,
    AUDIO_PITCH_SHIFTING,
};

static struct Option video_audio_opts[] =
{
    {
        OPT_SECTION,
        "VIDEO",
    },
    {
        OPT_CHOICE,
        "Render mode",
        "videomode", NULL,
        .choice =
        {
            vidmode_labels, vidmode_values,
            4, 3,
        },
    },
    {
        OPT_CHOICE,
        "Resolution",
        "screen_resolution", NULL,
        .choice =
        {
            resolutions, resolutions,
            8, 2,
        },
    },
    {
        OPT_CHOICE,
        "Scaling filter",
        "render_screen_filter", NULL,
        .choice =
        {
            scale_filter_labels, scale_filter_values,
            2, 0,
        },
    },
    {
        OPT_CHOICE,
        "Scaling mode",
        "render_screen_multiply", NULL,
        .choice =
        {
            scale_labels_software, scale_values_software,
            5, 1,
        },
    },
    {
        OPT_CHOICE,
        "MSAA",
        "render_multisampling", NULL,
        .choice =
        {
            multisampling, multisampling,
            3, 0,
        },
    },
    {
        OPT_CHOICE,
        "Sky render mode",
        "gl_skymode", NULL,
        .choice =
        {
            glsky_labels, glsky_values,
            4, 0,
        },
    },
    {
        OPT_SEPARATOR,
        "",
    },
    {
        OPT_SECTION,
        "AUDIO",
    },
    {
        OPT_CHOICE,
        "Sound type",
        "snd_pcspeaker", NULL,
        .choice =
        {
            sound_type_labels, sound_type_values,
            2, 0,
        },
    },
    {
        OPT_CHOICE,
        "MIDI player",
        "snd_midiplayer", NULL,
        .choice =
        {
            music_type_labels, music_type_values,
            2, 0,
        },
    },
    {
        OPT_FILE,
        "FluidSynth soundfont",
        "snd_soundfont", NULL,
        .file =
        {
            "",
            { "sf2", NULL },
        },
    },
    {
        OPT_CHOICE,
        "Sample rate",
        "samplerate", NULL,
        .choice =
        {
            sample_rates, sample_rates,
            4, 3,
        },
    },
    {
        OPT_INTEGER,
        "Sound channels",
        "snd_channels", NULL,
        .inum = { 1, 32, 1, 32 },
    },
    {
        OPT_BOOLEAN,
        "Random pitch shifting",
        "pitched_sounds", NULL,
        .boolean = 0,
    },
};

struct Menu ui_menu_video_audio =
{
    MENU_VIDEO_AUDIO,
    "Video/Audio",
    "Video/Audio",
    video_audio_opts,
    sizeof(video_audio_opts) / sizeof(*video_audio_opts),
    VIDEO_RENDER_MODE, 0,
    UI_MenuVideoAudio_Init,
    UI_MenuVideoAudio_Update,
    UI_MenuVideoAudio_Draw,
    UI_MenuVideoAudio_Reload,
};

static struct Menu *self = &ui_menu_video_audio;

static void UI_MenuVideoAudio_SetScalingChoices(int vitagl, int preserve_value)
{
    struct Option *opt = &video_audio_opts[VIDEO_SCALING];
    int old_value = -1;
    int i;

    if (preserve_value && opt->choice.val >= 0 &&
        opt->choice.val < opt->choice.count)
    {
        old_value = atoi(opt->choice.values[opt->choice.val]);
    }

    if (vitagl)
    {
        opt->choice.names = scale_labels_vitagl;
        opt->choice.values = scale_values_vitagl;
        opt->choice.count = 4;
    }
    else
    {
        opt->choice.names = scale_labels_software;
        opt->choice.values = scale_values_software;
        opt->choice.count = 5;
    }

    opt->choice.val = 0;
    if (preserve_value)
    {
        for (i = 0; i < opt->choice.count; ++i)
        {
            if (atoi(opt->choice.values[i]) == old_value)
            {
                opt->choice.val = i;
                break;
            }
        }
    }
}

static int UI_MenuVideoAudio_ProfileUsesVitaGL(void)
{
    char mode[128] = {0};

    if (CFG_ReadVar(ui_profile, "videomode", mode) != 0)
        return video_audio_opts[VIDEO_RENDER_MODE].choice.val == 3;

    return !strcmp(mode, "OpenGL");
}

static int UI_MenuVideo_ResolutionVisible(void)
{
    return video_audio_opts[VIDEO_RENDER_MODE].choice.val != 3;
}

static int UI_MenuVideo_ScalingFilterVisible(void)
{
    return video_audio_opts[VIDEO_RENDER_MODE].choice.val != 3;
}

static int UI_MenuVideo_VitaGLOnlyVisible(void)
{
    return video_audio_opts[VIDEO_RENDER_MODE].choice.val == 3;
}

void UI_MenuVideoAudio_Init(void)
{
    video_audio_opts[VIDEO_RESOLUTION].visible = UI_MenuVideo_ResolutionVisible;
    video_audio_opts[VIDEO_SCALING_FILTER].visible = UI_MenuVideo_ScalingFilterVisible;
    video_audio_opts[VIDEO_MSAA].visible = UI_MenuVideo_VitaGLOnlyVisible;
    video_audio_opts[VIDEO_SKY_MODE].visible = UI_MenuVideo_VitaGLOnlyVisible;
    video_audio_opts[AUDIO_SOUNDFONT].file.dir = FS_GetBaseDir();
    UI_MenuVideoAudio_SetScalingChoices(
        video_audio_opts[VIDEO_RENDER_MODE].choice.val == 3, 1);
}

void UI_MenuVideoAudio_Update(void)
{
    UI_MenuVideoAudio_SetScalingChoices(
        video_audio_opts[VIDEO_RENDER_MODE].choice.val == 3, 1);
}

void UI_MenuVideoAudio_Draw(void)
{
}

void UI_MenuVideoAudio_Reload(void)
{
    /* Select the valid list before OptsReload reads the new profile. */
    UI_MenuVideoAudio_SetScalingChoices(
        UI_MenuVideoAudio_ProfileUsesVitaGL(), 0);
}
