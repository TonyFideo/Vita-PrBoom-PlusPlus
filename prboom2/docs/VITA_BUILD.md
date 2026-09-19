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
  and configuration, without `doom1.wad`.

## Vita data layout

After installing the VPK, the launcher expects:

```text
ux0:/data/PrBoom++/
├── iwads/
│   └── doom*.wad
├── pwads/
│   └── *.wad
├── prboom-plus.cfg
└── prboom-plus.wad
```

The Doom IWAD is user-owned game data and is not included in this project.

## Reproducible local build

The repository contains only source and build inputs. Do not copy
`vitabuild/`, `vitabuild-ninja/`, `.elf`, `.self`, `.velf`, `.vpk`, object
files or logs into Git. They are generated locally and are covered by the
root `.gitignore`.
