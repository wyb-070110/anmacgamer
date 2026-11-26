#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
# Rebuilds the app icon from packaging/macos/icon/GameScrcpy.icon: draws the layers, compiles Assets.car (the
# Liquid Glass icon of macOS 26 and later) and GameScrcpy.icns (macOS 14 and 15) into src/app/resources, and
# exports the 1024 px preview. Needs Xcode 26 or later; the build itself only copies the results.
set -eu
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT

swift "$here/make_icon.swift" layers "$here/GameScrcpy.icon/Assets"
xcrun actool "$here/GameScrcpy.icon" --compile "$out" --platform macosx --minimum-deployment-target 14.0 \
    --app-icon GameScrcpy --output-partial-info-plist "$out/partial.plist" >/dev/null
cp "$out/Assets.car" "$root/src/app/resources/"

# actool's .icns stops at 256 px; macOS 14 and 15 get every size up to 1024 from Icon Composer's own renderer.
ictool="$(xcode-select -p)/../Applications/Icon Composer.app/Contents/Executables/ictool"
render() {
    "$ictool" "$here/GameScrcpy.icon" --export-image --output-file "$2" --platform macOS --rendition Default \
        --width "$1" --height "$1" --scale 1 >/dev/null
}
mkdir "$out/GameScrcpy.iconset"
for size in 16 32 128 256 512; do
    render "$size" "$out/GameScrcpy.iconset/icon_${size}x${size}.png"
    render $((size * 2)) "$out/GameScrcpy.iconset/icon_${size}x${size}@2x.png"
done
iconutil -c icns "$out/GameScrcpy.iconset" -o "$root/src/app/resources/GameScrcpy.icns"
render 1024 "$here/GameScrcpy-1024.png"
