#!/bin/bash
#
# Build neoglogg as an AppImage.
#
#     VERSION=1.2.3 ./release-appimage.sh
#
# Build this on the oldest distribution you intend to support: the AppImage
# bundles Qt and Boost but still links against the host glibc, which is not
# forward-compatible. The release workflow uses Ubuntu 22.04 for that reason.
#
# linuxdeploy and its Qt plugin are downloaded on first run.

set -e

if [ -z "$VERSION" ]; then
    VERSION=$(git describe --tags 2>/dev/null || echo "0.0.0")
    echo "No VERSION given, using $VERSION"
fi

ARCH=$(uname -m)
export ARCH

APPDIR=$PWD/AppDir
TOOLS=${TOOLS_DIR:-$PWD/.appimage-tools}

mkdir -p "$TOOLS"
fetch() {
    if [ ! -x "$TOOLS/$1" ]; then
        echo "Fetching $1"
        wget -q -O "$TOOLS/$1" "$2"
        chmod +x "$TOOLS/$1"
    fi
}
fetch linuxdeploy \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$ARCH.AppImage"
fetch linuxdeploy-plugin-qt \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$ARCH.AppImage"

rm -rf "$APPDIR"
make clean >/dev/null 2>&1 || true

qmake6 neoglogg.pro PREFIX=/usr VERSION="$VERSION"
make -j"$(nproc)"
make install INSTALL_ROOT="$APPDIR"

# linuxdeploy wants the desktop file and an icon it can find by name; both are
# already installed by the .pro into the AppDir, so point it at those.
"$TOOLS/linuxdeploy" \
    --appdir "$APPDIR" \
    --plugin qt \
    --desktop-file "$APPDIR/usr/share/applications/neoglogg.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/256x256/apps/neoglogg.png" \
    --output appimage

# linuxdeploy names the file from the desktop entry; make the version explicit.
built=$(ls -t neoglogg*.AppImage | head -1)
final="neoglogg-$VERSION-$ARCH.AppImage"
[ "$built" = "$final" ] || mv "$built" "$final"

echo "Built $final"
