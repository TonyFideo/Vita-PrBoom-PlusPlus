# PrBoom+ Vita

Port of PrBoom+ 2.6.66 for PlayStation Vita.

The Vita build uses the bundled `texture_matrix` VitaGL fork. The fork is
kept inside this repository so the build does not depend on a separately
installed or changing VitaGL checkout.

## Repository layout

- `src/` - PrBoom+ engine and GLBoom renderer.
- `vita/launcher/` - Vita WAD launcher.
- `vita/vitaGL/` - VitaGL fork used by this port.
- `vita/libtess/` - bundled tessellation library.
- `data/vita/` - Vita launcher assets and default configuration.
- `tests/` - upstream test fixtures.

Build directories, compiler outputs, logs and runtime dumps are intentionally
ignored. The parent workspace contains historical experiments that are not
part of this repository.

## Build

Build from a devkitPro MSYS2 shell with VitaSDK, CMake, Ninja, a host C
compiler and 7-Zip available. The exact command and Vita filesystem layout
are documented in [`docs/VITA_BUILD.md`](docs/VITA_BUILD.md).

The generated files are:

- `build-vita/src/prboom-plus.vpk`
- `build-vita/data.zip`

## Game data

This project does not distribute Doom IWADs. Put an IWAD that you own in the
Vita data directory, normally `ux0:/data/prboom/iwads/`. PWADs go in
`ux0:/data/prboom/pwads/`.

## License

PrBoom+ is distributed under the GNU General Public License; see `COPYING`.
The bundled VitaGL source retains its own license files under `vita/vitaGL/`.
