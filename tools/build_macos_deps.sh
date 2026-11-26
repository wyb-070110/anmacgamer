#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
# Builds GameScrcpy's own copies of FFmpeg (LGPL only), dav1d, SDL3, OpenSSL and libusb for macOS 14 and later,
# arm64 and x86_64, merged into universal libraries under deps/macos/universal. The app then ships these instead of
# Homebrew's (built for the build Mac's macOS only, and Homebrew's FFmpeg is GPL with x264/x265).
#
# FFmpeg's configuration matches the Windows build (third_party/ffmpeg/BUILDINFO.md) with VideoToolbox in place of
# D3D11VA: decoders h264, hevc, av1 (libdav1d), opus, aac; the mp4 muxer; the file protocol; nothing else.
#
#   tools/build_macos_deps.sh            sources in deps/macos/src (checked against the SHA-256 below)
#
# Needs: Xcode command line tools, nasm, meson, ninja, pkg-config (Homebrew), cmake.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
DEPS="$ROOT/deps/macos"
SRC="$DEPS/src"
MIN=14.0
ARCHES=(arm64 x86_64)
JOBS=$(sysctl -n hw.ncpu)

check() { # file sha256
    local sum
    sum=$(shasum -a 256 "$SRC/$1" | cut -d' ' -f1)
    [[ "$sum" == "$2" ]] || { echo "$1: checksum $sum, expected $2" >&2; exit 1; }
}
check ffmpeg-9.0.2.tar.xz 8c3850283eb25fa026482078a04051e0be17347b09ef81a0849bec15a96e002e
check dav1d-1.5.4.tar.xz 686616b7c69eb88d44459391ab25cac13b6647a3b288835c5784e71c1514a5c5
check SDL3-3.4.18.tar.gz 9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3
check libusb-1.0.30.tar.bz2 fea36f34f9156400209595e300840767ab1a385ede1dc7ee893015aea9c6dbaf
check openssl-3.5.9.tar.gz "$(awk '{print $1}' "$SRC/openssl-3.5.9.tar.gz.sha256")"

export MACOSX_DEPLOYMENT_TARGET=$MIN
WORK="$DEPS/work"
rm -rf "$WORK" "$DEPS/arm64" "$DEPS/x86_64" "$DEPS/universal"
mkdir -p "$WORK"

unpack() { # tarball → directory in $WORK/<arch>
    local dir="$WORK/$2"
    mkdir -p "$dir"
    tar -xf "$SRC/$1" -C "$dir"
}

for arch in "${ARCHES[@]}"; do
    PREFIX="$DEPS/$arch"
    mkdir -p "$PREFIX"
    host=$([[ $arch == arm64 ]] && echo aarch64 || echo x86_64)
    cflags="-arch $arch -mmacosx-version-min=$MIN -O2"

    echo "== dav1d ($arch)"
    unpack dav1d-1.5.4.tar.xz "$arch"
    cat > "$WORK/$arch/cross.txt" <<EOF
[binaries]
c = ['clang', '-arch', '$arch']
cpp = ['clang++', '-arch', '$arch']
ar = 'ar'
strip = 'strip'
nasm = 'nasm'
[built-in options]
c_args = ['-mmacosx-version-min=$MIN']
c_link_args = ['-mmacosx-version-min=$MIN']
[host_machine]
system = 'darwin'
cpu_family = '$([[ $arch == arm64 ]] && echo aarch64 || echo x86_64)'
cpu = '$([[ $arch == arm64 ]] && echo aarch64 || echo x86_64)'
endian = 'little'
EOF
    meson setup "$WORK/$arch/dav1d-build" "$WORK/$arch/dav1d-1.5.4" --cross-file "$WORK/$arch/cross.txt" \
        --prefix "$PREFIX" --libdir lib --buildtype release --default-library static \
        -Denable_tools=false -Denable_tests=false >/dev/null
    ninja -C "$WORK/$arch/dav1d-build" install >/dev/null

    echo "== FFmpeg ($arch)"
    unpack ffmpeg-9.0.2.tar.xz "$arch"
    (
        cd "$WORK/$arch/ffmpeg-9.0.2"
        PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig" ./configure --prefix="$PREFIX" \
            --enable-shared --disable-static --install-name-dir=@rpath \
            --arch="$arch" --target-os=darwin --enable-cross-compile --cc="clang -arch $arch" \
            --extra-cflags="-mmacosx-version-min=$MIN" --extra-ldflags="-arch $arch -mmacosx-version-min=$MIN" \
            --pkg-config=pkg-config --pkg-config-flags=--static \
            --disable-autodetect --disable-everything --disable-programs --disable-doc --disable-network \
            --disable-avdevice --disable-avfilter --disable-swscale \
            --enable-pthreads --enable-videotoolbox --enable-libdav1d \
            --enable-decoder=h264,hevc,av1,libdav1d,opus,aac \
            --enable-parser=h264,hevc,av1,opus,aac \
            --enable-hwaccel=h264_videotoolbox,hevc_videotoolbox,av1_videotoolbox \
            --enable-muxer=mp4 --enable-protocol=file >/dev/null
        make -j"$JOBS" >/dev/null
        make install >/dev/null
    )

    echo "== OpenSSL ($arch)"
    unpack openssl-3.5.9.tar.gz "$arch"
    (
        cd "$WORK/$arch/openssl-3.5.9"
        ./Configure "darwin64-$arch-cc" shared no-tests no-docs --prefix="$PREFIX" --libdir=lib \
            "-mmacosx-version-min=$MIN" >/dev/null
        make -j"$JOBS" >/dev/null
        make install_sw >/dev/null
    )

    echo "== libusb ($arch)"
    unpack libusb-1.0.30.tar.bz2 "$arch"
    (
        cd "$WORK/$arch/libusb-1.0.30"
        CC="clang" CFLAGS="$cflags" LDFLAGS="-arch $arch -mmacosx-version-min=$MIN" \
            ./configure --prefix="$PREFIX" --host="$host-apple-darwin" --enable-shared --disable-static \
            ac_cv_func_pipe2=no >/dev/null # pipe2 is only in macOS 27: on 14 the library would not load
        make -j"$JOBS" >/dev/null
        make install >/dev/null
    )
done

echo "== SDL3 (universal)"
UNI="$DEPS/universal"
mkdir -p "$UNI"
unpack SDL3-3.4.18.tar.gz sdl
cmake -S "$WORK/sdl/SDL3-3.4.18" -B "$WORK/sdl/build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=$MIN \
    -DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TESTS=OFF -DSDL_EXAMPLES=OFF \
    -DCMAKE_INSTALL_PREFIX="$UNI" >/dev/null
cmake --build "$WORK/sdl/build" >/dev/null
cmake --install "$WORK/sdl/build" >/dev/null

echo "== install names"
# OpenSSL and libusb name themselves by their absolute build path; the app looks them up by @rpath like the rest.
for arch in "${ARCHES[@]}"; do
    for lib in "$DEPS/$arch/lib/"*.dylib; do
        [[ -L "$lib" ]] && continue
        name=$(basename "$lib")
        changes=()
        for dep in $(otool -L "$lib" | awk -v d="$DEPS/$arch/lib/" 'index($1, d) == 1 {print $1}'); do
            changes+=(-change "$dep" "@rpath/$(basename "$dep")")
        done
        install_name_tool -id "@rpath/$name" "${changes[@]}" "$lib" 2>/dev/null
    done
done

echo "== merge arm64 + x86_64"
mkdir -p "$UNI/lib" "$UNI/include"
cp -R "$DEPS/arm64/include/." "$UNI/include/"
# OpenSSL's configuration header differs per arch: keep both and pick by the compiler's arch.
if [[ -f "$DEPS/arm64/include/openssl/configuration.h" ]]; then
    cp "$DEPS/arm64/include/openssl/configuration.h" "$UNI/include/openssl/configuration-arm64.h"
    cp "$DEPS/x86_64/include/openssl/configuration.h" "$UNI/include/openssl/configuration-x86_64.h"
    cat > "$UNI/include/openssl/configuration.h" <<'EOF'
#if defined(__aarch64__) || defined(__arm64__)
#include "configuration-arm64.h"
#else
#include "configuration-x86_64.h"
#endif
EOF
fi
for lib in "$DEPS/arm64/lib/"*.dylib; do
    name=$(basename "$lib")
    if [[ -L "$lib" ]]; then
        cp -P "$lib" "$UNI/lib/$name"
    else
        lipo -create "$lib" "$DEPS/x86_64/lib/$name" -output "$UNI/lib/$name"
    fi
done
mkdir -p "$UNI/lib/pkgconfig"
for pc in "$DEPS/arm64/lib/pkgconfig/"*.pc; do
    sed "s|$DEPS/arm64|$UNI|g" "$pc" > "$UNI/lib/pkgconfig/$(basename "$pc")"
done
# dav1d is linked into avcodec statically; its .pc is not needed by the app.
rm -f "$UNI/lib/pkgconfig/dav1d.pc"

echo "== check"
if otool -L "$UNI/lib/"*.dylib | grep -Eq "^[[:space:]]+$DEPS/(arm64|x86_64)/"; then
    echo "a library still names a build path" >&2
    exit 1
fi
for lib in "$UNI/lib/"*.dylib; do
    [[ -L "$lib" ]] && continue
    printf '%-32s %-14s macOS %s\n' "$(basename "$lib")" "$(lipo -archs "$lib")" \
        "$(otool -l "$lib" | awk '/LC_BUILD_VERSION/{getline; getline; getline; if ($1 == "minos") print $2}' | head -1)"
done
grep -h "^.*--enable-gpl\|--enable-nonfree" "$WORK/arm64/ffmpeg-9.0.2/ffbuild/config.mak" 2>/dev/null && \
    { echo "FFmpeg was configured with GPL or nonfree parts" >&2; exit 1; }
grep -q 'CONFIG_GPL 0' "$WORK/arm64/ffmpeg-9.0.2/config.h" && echo "FFmpeg: LGPL (CONFIG_GPL 0)"
echo "done: $UNI"
