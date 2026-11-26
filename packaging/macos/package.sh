#!/bin/bash
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
# Makes an anmacgamer .dmg for macOS from the built app (cmake --build --preset mac-release, or mac-app):
#   - copies build/<preset>/bin/anmacgamer.app to build/package/,
#   - puts adb inside (Contents/Resources/adb), so a Mac without the Android tools can connect phones,
#   - brings in Qt (frameworks, QML, plugins) and the other libraries the app and its helpers use (FFmpeg, SDL3,
#     libusb, OpenSSL…) with macdeployqt, so nothing points at ~/Qt or /opt/homebrew any more,
#   - signs everything ad hoc (no Apple developer account: testers open it with right-click → Open the first time),
#   - makes anmacgamer-<version>-<label>-macos-<arch>.dmg with the app and a link to Applications.
#
#   packaging/macos/package.sh [label]        (label defaults to "beta1")
#
# Environment: PRESET (default mac-release: universal, macOS 14 and later, the app's own LGPL FFmpeg),
# QT_DIR (default ~/Qt/6.10.3/macos), ADB (default: adb on PATH, then the Android SDK's).
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
LABEL=${1:-beta1}
PRESET=${PRESET:-mac-release}
QT_DIR=${QT_DIR:-$HOME/Qt/6.10.3/macos}
SOURCE="$ROOT/build/$PRESET/bin/anmacgamer.app"
OUT="$ROOT/build/package"
APP="$OUT/anmacgamer.app"

[[ -d "$SOURCE" ]] || { echo "build the app first: cmake --build --preset $PRESET" >&2; exit 1; }
VERSION=$(/usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" "$SOURCE/Contents/Info.plist")
ARCH=$(lipo -archs "$SOURCE/Contents/MacOS/anmacgamer" | tr ' ' '-')
[[ "$ARCH" == "x86_64-arm64" || "$ARCH" == "arm64-x86_64" ]] && ARCH=universal
DMG="$OUT/anmacgamer-$VERSION-$LABEL-macos-$ARCH.dmg"

echo "== anmacgamer $VERSION ($LABEL, $ARCH)"
rm -rf "$OUT"
mkdir -p "$OUT"
cp -R "$SOURCE" "$APP"

echo "== adb"
ADB=${ADB:-$(command -v adb || true)}
[[ -x "${ADB:-}" ]] || ADB="$HOME/Library/Android/sdk/platform-tools/adb"
[[ -x "$ADB" ]] || { echo "adb not found: set ADB=/path/to/adb" >&2; exit 1; }
ADB=$(python3 -c 'import os,sys; print(os.path.realpath(sys.argv[1]))' "$ADB")
mkdir -p "$APP/Contents/Resources/adb"
cp "$ADB" "$APP/Contents/Resources/adb/adb"
for notice in NOTICE.txt NOTICE; do
    [[ -f "$(dirname "$ADB")/$notice" ]] && cp "$(dirname "$ADB")/$notice" "$APP/Contents/Resources/adb/NOTICE.txt" && break
done
"$APP/Contents/Resources/adb/adb" version | head -1

echo "== Qt and libraries (macdeployqt)"
"$QT_DIR/bin/macdeployqt" "$APP" \
    -qmldir="$ROOT/src" \
    -executable="$APP/Contents/MacOS/gs-exthost" \
    -executable="$APP/Contents/MacOS/gs-scripthost" \
    -executable="$APP/Contents/MacOS/gs" \
    -always-overwrite 2>&1 | grep -vE "^(Log|$)" || true

# Only SQLite is used; the other database drivers point at client libraries a Mac does not have.
find "$APP/Contents/PlugIns/sqldrivers" -type f ! -name 'libqsqlite.dylib' -delete 2>/dev/null || true

# Search paths from the build machine (Homebrew, Qt) must not be searched on a tester's Mac.
find "$APP/Contents" -type f \( -perm -111 -o -name '*.dylib' \) -print0 | while IFS= read -r -d '' binary; do
    otool -l "$binary" 2>/dev/null | awk '/LC_RPATH/{getline; getline; print $2}' | while read -r rpath; do
        case "$rpath" in
        /opt/homebrew/* | /usr/local/* | "$HOME"/*) install_name_tool -delete_rpath "$rpath" "$binary" 2>/dev/null || true ;;
        esac
    done
done

echo "== nothing may point outside the app"
LEFT=$(find "$APP/Contents" -type f \( -perm -111 -o -name '*.dylib' \) -print0 \
    | xargs -0 -n1 otool -L 2>/dev/null | grep -E "/opt/homebrew|/usr/local/(opt|Cellar)|$HOME/Qt" || true)
if [[ -n "$LEFT" ]]; then
    echo "$LEFT" >&2
    echo "some libraries still point outside the app" >&2
    exit 1
fi

echo "== minimum macOS"
# The libraries from Homebrew are built for the build machine's macOS: the app needs the newest of them.
MINOS=$(find "$APP/Contents" -type f \( -perm -111 -o -name '*.dylib' \) -print0 \
    | xargs -0 -n1 otool -l 2>/dev/null | awk '/LC_BUILD_VERSION/{getline; getline; getline; if ($1 == "minos") print $2}' \
    | sort -V | tail -1)
/usr/libexec/PlistBuddy -c "Set :LSMinimumSystemVersion $MINOS" "$APP/Contents/Info.plist"
echo "needs macOS $MINOS"

echo "== sign (ad hoc)"
xattr -cr "$APP"
codesign --force --deep --sign - "$APP"
codesign --verify --deep --strict "$APP"

echo "== dmg"
STAGE="$OUT/dmg"
mkdir -p "$STAGE"
cp -R "$APP" "$STAGE/"
ln -s /Applications "$STAGE/Applications"
if [[ -f "$APP/Contents/Resources/GameScrcpy.icns" && -x /usr/bin/SetFile ]]; then
    cp "$APP/Contents/Resources/GameScrcpy.icns" "$STAGE/.VolumeIcon.icns"
    /usr/bin/SetFile -a V "$STAGE/.VolumeIcon.icns"
    /usr/bin/SetFile -a C "$STAGE"
fi
hdiutil create -quiet -volname "anmacgamer $VERSION $LABEL" -srcfolder "$STAGE" -ov -format UDZO "$DMG"
rm -rf "$STAGE"
du -h "$DMG"
echo "$DMG"
