# DUMB in Vita-PrBoom++

This directory contains the DUMB 1.0.0 source used by the Vita build for
tracker music playback (MOD, IT, XM and S3M formats).

The source and license are preserved from the DUMB distribution.  The
project-specific CMake integration is separate from the upstream ZMusic build
file: Vita is compiled without `_USE_SSE`/`-msse`, so DUMB uses its portable
scalar ARM path.  See `licence.txt` for the original license.
