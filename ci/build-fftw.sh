#!/usr/bin/env bash
# Build a static, position independent single precision FFTW (libfftw3f.a)
# and install it into the prefix given as first argument.
# Used by the wheel builds on Linux (inside the manylinux container) and macOS.
set -euo pipefail

PREFIX=${1:?usage: build-fftw.sh <prefix>}
VERSION=3.3.11
SHA256=5630c24cdeb33b131612f7eb4b1a9934234754f9f388ff8617458d0be6f239a1

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
cd "$WORK"

curl -fsSL -o fftw.tar.gz "https://fftw.org/fftw-$VERSION.tar.gz"
echo "$SHA256  fftw.tar.gz" | (sha256sum -c - 2>/dev/null || shasum -a 256 -c -)
tar xzf fftw.tar.gz
cd "fftw-$VERSION"

# FFTW dispatches between the enabled SIMD code paths at run time,
# so the AVX code is only used on CPUs that support it.
case "$(uname -m)" in
  x86_64|amd64) SIMD="--enable-sse2 --enable-avx --enable-avx2" ;;
  arm64|aarch64) SIMD="--enable-neon" ;;
  *) SIMD="" ;;
esac

./configure --prefix="$PREFIX" --enable-float --enable-static --disable-shared --with-pic \
  --disable-fortran --disable-doc $SIMD
make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu)"
make install
