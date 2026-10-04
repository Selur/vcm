# vcm

VapourSynth plugin by V.C.Mohan with several pixel modifying, pixel moving,
frequency domain and test pattern functions. This is a maintained fork of
<http://www.avisynth.nl/users/vcmohan/vcm/vcm.html>
(git mirror: <https://github.com/Vapoursynth-Plugins-Gitify/vcm>).

Functions: `Amp`, `Bokeh`, `Circles`, `ColorBox`, `DeBarrel`, `DeJitter`,
`F1QClean`, `F1QLimit`, `F1Quiver`, `F2QBlur`, `F2QBokeh`, `F2QCorr`,
`F2QLimit`, `F2QSharp`, `F2Quiver`, `Fan`, `Fisheye`, `GBlur`, `Grid`,
`Hist`, `Jitter`, `MBlur`, `Mean`, `Median`, `Neural`, `Pattern`, `Reform`,
`Rotate`, `SaltPepper`, `StepFilter`, `Variance`, `Veed`.
The documentation of every function is in the [docs](docs/vcm.html) folder.

Changes compared to the original:

- ported to the VapourSynth API 4 (VapourSynth R55 or newer)
- `meson` based build for Windows, Linux and macOS, `fftw3f` is linked statically
- several bug fixes (see the git history)

## Installation

Prebuilt wheels for Windows x64, Linux x86_64 and macOS arm64 are attached to
each [GitHub release](https://github.com/Selur/vcm/releases):

    pip install vapoursynth_vcm-*.whl

The wheel installs the plugin next to the `vapoursynth` Python package, from
where it is autoloaded. Plain plugin binaries (`vcm.dll`, `libvcm.so`,
`libvcm.dylib`) for the usual plugin autoload folders are attached as zip
archives as well.

## Testing

`test/test_vcm.py` calls every function on small synthetic clips in several
formats and checks the results. It needs the `vapoursynth` Python module and
`numpy`:

    python3 test/test_vcm.py build/libvcm.so

## Compilation

Meson, Ninja and `fftw3f` (single precision FFTW 3) are required. The
VapourSynth API 4 headers are bundled, a system installation of VapourSynth is
optional.

    meson setup build
    ninja -C build

`-Dprefer_static=true` links the static `libfftw3f`. When fftw is not found by
pkg-config, point `-Dfftw3_dir=<prefix>` at a directory containing
`include/fftw3.h` and `lib/libfftw3f.*` (e.g. a vcpkg triplet directory on
Windows). `ci/build-fftw.sh <prefix>` builds a suitable static fftw3f from
source on Linux and macOS.

On macOS the plugin is built as `libvcm.dylib`, which is the only extension
VapourSynth autoloads there.

## Plugin Author

V. C. Mohan - http://www.avisynth.nl/users/vcmohan/

## License

GNU GPL v3, see [LICENSE](LICENSE). The bundled VapourSynth headers are
LGPL 2.1 (see `include/vapoursynth/LICENSE.VapourSynth`).
