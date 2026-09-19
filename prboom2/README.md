# Vita-PrBoom++ (PrBoom+ 2.6.66 for the PSVITA)

This migration tree uses the current VitaGL source in `vita/vitaGL-modern`,
with `ENABLE_LEGACY_PIPELINE=1` for PrBoom+'s existing client-array renderer.
The old `texture_matrix` fork remains in `vita/vitaGL` only as a reference.

## Features

- Updated to mainstream 2.6.66.
- Vita features (WIP).
- Made in mind for Community WADs.
- Support for software (WIP performance) and OpenGL (current VitaGL).
- More to come...

## Game data

This project doesn't distribute Doom WADs. Put your IWAD inside `data/PrBoom++/iwads/`. PWADs go in
`data/PrBoom++/pwads/` (It supports data files inside ux0/ur0/uma0).

## Vita build dependencies

The current VitaGL build also needs the bundled `vita/vitaShaRK` source and
`SceShaccCgExt`. The generated `taihen_stub` archive is only an import library
for linking; the real taiHEN runtime must still be available on the Vita.
`libshacccg.suprx` is likewise a device-side runtime prerequisite.

The CMake options `VITA_SCE_SHACCCG_EXT_LIBRARY` and
`VITA_TAIHEN_STUB_LIBRARY` accept local archives when they are not installed
in VitaSDK. The final VPK target is `vita-prboom++.vpk-vpk`, and the separate
`data.zip` target intentionally omits `doom1.wad`.

## License

PrBoom+ is distributed under the GNU General Public License; see `COPYING`.
The bundled VitaGL sources retain their own license files under `vita/vitaGL/`
and `vita/vitaGL-modern/`.
