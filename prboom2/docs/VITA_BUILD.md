# Vita build instructions

Run these commands from the repository root in a devkitPro MSYS2 shell.

```sh
export VITASDK=/usr/local/vitasdk
export PATH="$VITASDK/bin:$PATH"

cmake -S . -B build-vita -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$VITASDK/share/vita.toolchain.cmake" \
  -DVITA=ON \
  -DBUILD_GL=ON \
  -DVITA_HOST_CC=/c/msys64/ucrt64/bin/gcc.exe \
  -DVITA_ZIP_TOOL=/c/msys64/usr/lib/p7zip/7z.exe

cmake --build build-vita --target vita-prboom++.vpk-vpk
```

To bundle the local Freedoom 0.13.0 IWADs and their attribution files in
`data.zip`, add the directory containing `freedoom1.wad`, `freedoom2.wad`,
`COPYING.txt`, `CREDITS.txt` and `CREDITS-MUSIC.txt`:

```sh
  -DVITA_FREEDOOM_DIR=/path/to/freedoom-0.13.0
```

The project requires CMake 3.5 or newer.  The VitaSDK toolchain also needs to
be visible through `VITASDK`, and its `bin` directory must be on `PATH` because
the build invokes the bundled VitaGL Makefile.

`VITA_HOST_CC` and `VITA_ZIP_TOOL` are host paths. Change them to the paths
used by the local MSYS2 installation if they differ.

The build invokes the Makefile in `vita/vitaGL-modern/`, builds the bundled
`vitaShaRK`, and links the resulting `libvitaGL.a` into GLBoom. The legacy
`texture_matrix` VitaGL fork is not part of this release build. If
`SceShaccCgExt` and `taihen_stub` are not installed in VitaSDK, pass their
locally built static-library paths with `VITA_SCE_SHACCCG_EXT_LIBRARY` and
`VITA_TAIHEN_STUB_LIBRARY`.

Outputs:

- `build-vita/src/vita-prboom++.vpk` - installable Vita package.
- `build-vita/data.zip` - data package containing the internal PrBoom+ data
  and configuration, without `doom1.wad`. When `VITA_FREEDOOM_DIR` is set,
  it also contains `freedoom1.wad`, `freedoom2.wad` and their attribution
  files under `data/PrBoom++/iwads/` and `data/PrBoom++/licenses/`.

## Vita data layout

After installing the VPK, the launcher expects:

```text
ux0:/data/PrBoom++/
├── iwads/
│   ├── doom*.wad
│   ├── freedoom1.wad
│   └── freedoom2.wad
├── pwads/
│   └── *.wad
├── licenses/
│   ├── Freedoom-COPYING.txt
│   ├── Freedoom-CREDITS.txt
│   └── Freedoom-CREDITS-MUSIC.txt
├── prboom-plus.cfg
└── prboom-plus.wad
```

The Doom IWAD is user-owned game data and is not included in this project.

## Reproducible local build

The repository contains only source and build inputs. Do not copy
`vitabuild/`, `vitabuild-ninja/`, `.elf`, `.self`, `.velf`, `.vpk`, object
files or logs into Git. They are generated locally and are covered by the
root `.gitignore`.
