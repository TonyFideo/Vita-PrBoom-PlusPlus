#include "utils.h"
#include "files.h"
#include "net.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <vitasdk.h>

#define mkdir(x, y) sceIoMkdir(x, y)

char fs_base_dir[MAX_FNAME];
char fs_pwad_dir[MAX_FNAME];
char fs_demo_dir[MAX_FNAME];
char fs_temp_dir[MAX_FNAME];
char fs_iwad_dir[MAX_FNAME];

// default profiles

struct Profile fs_profiles[MAX_PROFILES] =
{
    // doom1-based
    { "Doom (Shareware)", "doom1.wad", 1 },
    { "Doom", "doom.wad", 1 },
    { "FreeDM", "freedm.wad", 1 },
    { "FreeDoom: Phase 1", "freedoom1.wad", 1 },
    { "Chex Quest", "chex.wad", 1 },
    // doom2-based
    { "Doom II", "doom2.wad", 0 },
    { "Final Doom: TNT Evilution", "tnt.wad", 0 },
    { "Final Doom: The Plutonia Experiment", "plutonia.wad", 0 },
    { "FreeDoom: Phase 2", "freedoom2.wad", 0 },
    { "Hacx", "hacx.wad", 0 },
};

static char fs_error[4096];

static void SetError(const char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    vsnprintf(fs_error, sizeof(fs_error), fmt, argptr);
    va_end(argptr);
}

static int CheckForProfile(int g)
{
    char buf[MAX_FNAME];
    snprintf(buf, sizeof(buf), "%s/%s", fs_iwad_dir, fs_profiles[g].iwad);
    return fexists(buf);
}

static const char *FindVitaDataDir(void)
{
    static char base[MAX_FNAME];
    const char *drives[] = { "uma0", "imc0", "ux0" };

    if (base[0]) return base;

    /* Prefer a complete data directory so a partial uma0 installation does
       not hide a valid installation on another Vita storage device. */
    for (unsigned int i = 0; i < sizeof(drives) / sizeof(*drives); ++i)
    {
        char wad[MAX_FNAME];
        snprintf(base, sizeof(base), "%s:/data/PrBoom++", drives[i]);
        snprintf(wad, sizeof(wad), "%s/prboom-plus.wad", base);
        if (fexists(wad)) return base;
    }

    /* Keep the original behavior before data.zip has been installed. */
    for (unsigned int i = 0; i < sizeof(drives) / sizeof(*drives); ++i)
    {
        snprintf(base, sizeof(base), "%s:/data/PrBoom++", drives[i]);
        if (isdir(base)) return base;
    }

    snprintf(base, sizeof(base), "ux0:/data/PrBoom++");
    return base;
}

const char *FS_GetBaseDir(void)
{
    if (fs_base_dir[0] == 0)
        snprintf(fs_base_dir, sizeof(fs_base_dir), "%s", FindVitaDataDir());
    return fs_base_dir;
}

int FS_Init(void)
{
    char fpath[MAX_FNAME];

    snprintf(fs_pwad_dir, sizeof(fs_pwad_dir), "%s/pwads", FS_GetBaseDir());
    snprintf(fs_demo_dir, sizeof(fs_demo_dir), "%s/demos", FS_GetBaseDir());
    snprintf(fs_iwad_dir, sizeof(fs_iwad_dir), "%s/iwads", FS_GetBaseDir());
    snprintf(fs_temp_dir, sizeof(fs_temp_dir), "%s/tmp", FS_GetBaseDir());

    mkdir(FS_GetBaseDir(), 0755);
    mkdir(fs_pwad_dir, 0755);
    mkdir(fs_demo_dir, 0755);
    mkdir(fs_temp_dir, 0755);

    if (FS_LoadProfiles() < 0) return -1;

    int numgames = 0;
    for (int i = 0; i < MAX_PROFILES; ++i)
    {
        if (fs_profiles[i].iwad[0] == 0) break;
        int present = CheckForProfile(i);
        fs_profiles[i].present = present;
        fs_profiles[i].monsters[0] = '0';
        fs_profiles[i].skill[0] = '0';
        fs_profiles[i].complevel = -1;
        snprintf(fs_profiles[i].joinaddr, MAX_FNAME, "%s:5030", net_my_ip);
        numgames += present;
    }

    if (!numgames)
    {
        SetError("No supported IWADs were found in the `iwads` directory.\n" \
                 "Make sure you have installed at least one game properly.");
        return -1;
    }

    return 0;
}

void FS_Free(void)
{

}

char *FS_Error(void)
{
    return fs_error;
}

int FS_ProfileAvailable(int profile)
{
    if (profile < 0 || profile >= MAX_PROFILES) return 0;
    return fs_profiles[profile].present;
}

int FS_ListDir(struct FileList *flist, const char *path, const char *ext)
{
    static char buf[512];

    DIR *dp = opendir(path);
    if (!dp) return -1;

    struct dirent *ep;
    while (ep = readdir(dp))
    {
        if (!ep->d_name || ep->d_name[0] == '.')
            continue;

        char *fname = ep->d_name;
        snprintf(buf, sizeof(buf), "%s/%s", path, fname);

        if (!isdir(buf) && !strcasecmp(fext(fname), ext))
        {
            flist->files = realloc(flist->files, (flist->numfiles + 1) * sizeof(char*));
            if (!flist->files) return -2;
            flist->files[flist->numfiles] = strdup(fname);
            flist->numfiles++;
        }
    }

    closedir(dp);

    return flist->numfiles;
}

void FS_FreeFileList(struct FileList *flist)
{
    for (int i = 0; i < flist->numfiles; ++i)
      free(flist->files[i]);

    free(flist->files);
    flist->files = NULL;
    flist->numfiles = 0;
}

int FS_LoadProfiles(void)
{
    char fname[MAX_FNAME];
    struct Profile prof = { { 0 } };
    char keybuf[512] = { 0 };
    char *value;
    const char *key;

    snprintf(fname, sizeof(fname), "%s/profiles.cfg", FS_GetBaseDir());

    FILE *f = fopen(fname, "r");
    if (!f)
    {
        // this is not an error, we'll create one on exit
        return 0;
    }

    int num_profiles = 0;

    memset(fs_profiles, 0, sizeof(fs_profiles));

    while (fgets(keybuf, sizeof(keybuf), f))
    {
        char *line = keybuf;

        while (*line && isspace((unsigned char)*line)) ++line;
        if (!*line || *line == '#')
            continue;

        value = line;
        while (*value && !isspace((unsigned char)*value)) ++value;
        if (!*value)
            continue;

        *value++ = '\0';
        while (*value && isspace((unsigned char)*value)) ++value;

        char *end = value + strlen(value);
        while (end > value && isspace((unsigned char)end[-1])) --end;
        *end = '\0';

        key = line;

        if (!strcmp(key, "profile"))
        {
            // A new record starts here. Discard any incomplete previous one.
            memset(&prof, 0, sizeof(prof));
            strncpy(prof.name, value, sizeof(prof.name) - 1);
        }
        else if (!strcmp(key, "iwad"))
        {
            strncpy(prof.iwad, value, sizeof(prof.iwad) - 1);
        }
        else if (!strcmp(key, "episodic"))
        {
            prof.episodic = (value[0] == '1');
        }
        else if (!strcmp(key, "file"))
        {
            for (int i = 0; i < MAX_FILES; ++i)
            {
                if (prof.files[i][0] == 0)
                {
                    strncpy(prof.files[i], value, sizeof(prof.files[i]) - 1);
                    break;
                }
            }
        }
        else if (!strcmp(key, "rsp"))
        {
            strncpy(prof.rsp, value, sizeof(prof.rsp) - 1);
        }
        else if (!strcmp(key, "nodeh"))
        {
            prof.nodeh = (value[0] == '1');
        }
        else if (!strcmp(key, "logfile"))
        {
            prof.logfile = (value[0] == '1');
        }
        else if (!strcmp(key, "logadvanced"))
        {
            prof.log_advanced = (value[0] == '1');
        }
#ifdef HAVE_PROFILING
        else if (!strcmp(key, "logprofiling"))
        {
            prof.log_profiling = (value[0] == '1');
        }
#endif
        else if (!strcmp(key, "logrender"))
        {
            prof.log_render = (value[0] == '1');
        }
        else if (!strcmp(key, "end") && !strcmp(value, "profile"))
        {
            if (prof.iwad[0] && prof.name[0])
            {
                memcpy(fs_profiles + num_profiles++, &prof, sizeof(prof));
                memset(&prof, 0, sizeof(prof));
            }
            else
            {
                SetError("Invalid profile record (#%d) in file\n%s", num_profiles, fname);
                fclose(f);
                return -1;
            }
        }
    }

    fclose(f);

    return num_profiles;
}

int FS_SaveProfiles(void)
{
    char fname[MAX_FNAME];

    snprintf(fname, sizeof(fname), "%s/profiles.cfg", FS_GetBaseDir());

    FILE *f = fopen(fname, "w");
    if (!f)
    {
        SetError("Could not write file:\n%s", fname);
        return -1;
    }

    for (int i = 0; i < MAX_PROFILES; ++i)
    {
        if (!fs_profiles[i].iwad[0]) break;
        fprintf(f, "profile %s\n", fs_profiles[i].name);
        fprintf(f, "  iwad %s\n", fs_profiles[i].iwad);
        if (fs_profiles[i].episodic) fprintf(f, "  episodic 1\n");
        if (fs_profiles[i].rsp[0]) fprintf(f, "  rsp %s\n", fs_profiles[i].rsp);
        if (fs_profiles[i].nodeh) fprintf(f, "  nodeh 1\n");
        if (fs_profiles[i].logfile) fprintf(f, "  logfile 1\n");
        if (fs_profiles[i].log_advanced) fprintf(f, "  logadvanced 1\n");
#ifdef HAVE_PROFILING
        if (fs_profiles[i].log_profiling) fprintf(f, "  logprofiling 1\n");
#endif
        if (fs_profiles[i].log_render) fprintf(f, "  logrender 1\n");
        for (int j = 0; j < MAX_FILES; ++j)
        {
            if (fs_profiles[i].files[j][0])
                fprintf(f, "  file %s\n", fs_profiles[i].files[j]);
        }
        fprintf(f, "end profile\n\n");
    }

    fclose(f);

    return 0;
}

static void WriteResponseFile(int profile, const char *fname)
{
    struct Profile *g = fs_profiles + profile;

    FILE *f = fopen(fname, "w");
    if (!f) I_Error("Could not create file\n%s", fname);

    if (g->netmode)
    {
        if (g->netmode == NETMODE_CLIENT)
            fprintf(f, "-net %s\n", g->joinaddr);
        else
            fprintf(f, "-solo-net\n");
    }

    fprintf(f, "-iwad %s\n", g->iwad);

    const char *pwads[MAX_FILES];
    int numpwads = 0;
    const char *dehs[MAX_FILES];
    int numdehs = 0;

    for (int i = 0; i < MAX_FILES; ++i)
    {
        if (!g->files[i][0]) continue;
        if (!fextcmp(g->files[i], "deh") || !fextcmp(g->files[i], "bex"))
            dehs[numdehs++] = g->files[i];
        else
            pwads[numpwads++] = g->files[i];
    }

    if (numpwads)
    {
        fprintf(f, "-file");
        for (int i = 0; i < numpwads; ++i)
            fprintf(f, " %s", pwads[i]);
        fprintf(f, "\n");
    }

    if (numdehs)
    {
        fprintf(f, "-deh");
        for (int i = 0; i < numdehs; ++i)
            fprintf(f, " %s", dehs[i]);
        fprintf(f, "\n");
    }

    if (g->nodeh)
        fprintf(f, "-nodeh\n");

    if (g->skill[0] && g->skill[0] != '0')
        fprintf(f, "-skill %s\n", g->skill);

    if (g->complevel >= 0)
        fprintf(f, "-complevel %d\n", g->complevel);

    if (g->warp)
    {
        if (g->episodic)
        {
            int ep = g->warp / 10;
            int map = g->warp % 10;
            if (!ep) ep = 1;
            if (!map) map = 1;
            fprintf(f, "-warp %d %d\n", ep, map);
        }
        else
        {
            fprintf(f, "-warp %d\n", g->warp);
        }
    }

    if (g->monsters[0] == '1')
        fprintf(f, "-nomonsters\n");
    else if (g->monsters[0] == '2')
        fprintf(f, "-fast\n");
    else if (g->monsters[0] == '4')
        fprintf(f, "-respawn\n");
    else if (g->monsters[0] == '6')
        fprintf(f, "-fast\n-respawn\n");

    if (g->record)
        fprintf(f, "-record %s/mydemo\n", fs_demo_dir);
    else if (g->demo[0])
        fprintf(f, "-playdemo %s/%s\n", fs_demo_dir, g->demo);

    /* The existing Debug logging option owns the legacy -logfile switch.
       The other switches are independent categories and are consumed by the
       Vita runtime without implicitly enabling basic logging. */
    if (g->logfile)
        fprintf(f, "-logfile\n");
    if (g->log_advanced)
        fprintf(f, "-logadvanced\n");
#ifdef HAVE_PROFILING
    if (g->log_profiling)
        fprintf(f, "-logprofiling\n");
#endif
    if (g->log_render)
        fprintf(f, "-logrender\n");

    fclose(f);
}

void FS_ExecGame(int profile)
{
    if (profile < 0 || profile >= MAX_PROFILES) return;

    struct Profile *g = fs_profiles + profile;
    static char rsp[MAX_FNAME];
    static char exe[128];
    static char *argv[3];

    if (g->rsp[0])
    {
        snprintf(rsp, sizeof(rsp), "@%s/%s", fs_pwad_dir, g->rsp);
    }
    else
    {
        rsp[0] = '@';
        snprintf(rsp + 1, sizeof(rsp) - 1, "%s/prboom.rsp", fs_temp_dir);
        WriteResponseFile(profile, rsp + 1);
    }

    snprintf(exe, sizeof(exe), "app0:/prboom.bin");

    argv[0] = exe;
    argv[1] = rsp;
    argv[2] = NULL;

    printf("FS_ExecGame: launching %s with %s\n", exe, rsp);
    fflush(stdout);
    /* The process is replaced on success. Freeing vita2d/GXM immediately
       before LoadExec makes current Vita3K dereference destroyed renderer
       state, and is unnecessary on hardware as well. */
    printf("FS_ExecGame: calling sceAppMgrLoadExec\n");
    fflush(stdout);

    /* A successful LoadExec schedules replacement of this process.  Do not
       call sceKernelExitProcess() after it: Vita3K uses the pending
       LoadExec request when it handles the process exit, and a second plain
       exit would discard the requested target and close the launcher only. */
    int rc = sceAppMgrLoadExec(exe, argv, NULL);
    printf("FS_ExecGame: sceAppMgrLoadExec returned 0x%08X\n", rc);
    fflush(stdout);

    {
        char fname[MAX_FNAME];
        FILE *ferr;

        snprintf(fname, sizeof(fname), "%s/error.log", FS_GetBaseDir());
        ferr = fopen(fname, "w");
        if (ferr)
        {
            fprintf(ferr, "sceAppMgrLoadExec() returned 0x%08X\n", rc);
            fclose(ferr);
        }
    }
    /* On hardware LoadExec does not return.  Vita3K returns after queuing the
       relaunch, so returning here preserves that pending request. */
}
