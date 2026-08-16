#!/bin/bash
#
# Regenerate every icon in the tree from the single source SVG.
#
# Run this after editing images/hicolor/scalable/neoglogg.svg; nothing else
# is hand-maintained. Requires inkscape and ImageMagick; optipng is used if
# it happens to be installed.
#
#     ./make-icons.sh
#
# Produces:
#   images/hicolor/{64x64,128x128,256x256}/neoglogg.png  - theme + Qt resource
#   neoglogg.ico                                         - Windows (RC_ICONS)
#   images/neoglogg.iconset/                             - macOS, for iconutil
#   images/neoglogg.icns                                 - macOS bundle icon

set -e

SVG=images/hicolor/scalable/neoglogg.svg
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

if [ ! -f "$SVG" ]; then
    echo "$SVG not found; run this from the top of the source tree." >&2
    exit 1
fi

render() {
    # render <size> <output>
    inkscape "$SVG" -o "$2" -w "$1" -h "$1" >/dev/null 2>&1
    if [ `which optipng` ]; then
        optipng -quiet -o5 "$2"
    fi
}

echo "Rendering from $SVG"

# Sizes below 64 are deliberately not shipped as loose PNGs: downscaling this
# artwork that far produces the same blur the toolkit would produce anyway.
# They are embedded in the .ico and .icns only, where the platform reaches for
# them directly (Windows title bars, Finder list view).
for size in 64 128 256; do
    mkdir -p images/hicolor/${size}x${size}
    render $size images/hicolor/${size}x${size}/neoglogg.png
    echo "  hicolor ${size}x${size}"
done

# Windows: one multi-resolution .ico
for size in 16 32 48 64 128 256; do
    render $size "$WORK/ico_$size.png"
done
convert "$WORK/ico_16.png" "$WORK/ico_32.png" "$WORK/ico_48.png" \
        "$WORK/ico_64.png" "$WORK/ico_128.png" "$WORK/ico_256.png" \
        neoglogg.ico
echo "  neoglogg.ico (16 32 48 64 128 256)"

# macOS: the .iconset is what `iconutil -c icns` consumes on a Mac, and is
# kept so the bundle icon can be rebuilt there without this script.
mkdir -p images/neoglogg.iconset
for size in 16 32 128 256 512; do
    render $size images/neoglogg.iconset/icon_${size}x${size}.png
    render $((size * 2)) images/neoglogg.iconset/icon_${size}x${size}@2x.png
done
echo "  images/neoglogg.iconset"

# ...and the .icns itself, written directly so that this works off a Mac too.
# PNG-backed ICNS entries are understood by macOS 10.7 and later.
for size in 16 32 64 128 256 512 1024; do
    render $size "$WORK/icns_$size.png"
done
python3 - "$WORK" images/neoglogg.icns <<'PY'
import struct, sys, os

work, out = sys.argv[1], sys.argv[2]

# OSType -> pixel dimension. The icp* and ic0* entries are the 1x ladder;
# ic11..ic14 are the retina variants macOS asks for on HiDPI displays.
entries = [
    ("icp4", 16), ("icp5", 32), ("icp6", 64),
    ("ic07", 128), ("ic08", 256), ("ic09", 512), ("ic10", 1024),
    ("ic11", 32), ("ic12", 64), ("ic13", 256), ("ic14", 512),
]

chunks = b""
for ostype, size in entries:
    with open(os.path.join(work, "icns_%d.png" % size), "rb") as f:
        png = f.read()
    chunks += ostype.encode("ascii") + struct.pack(">I", len(png) + 8) + png

with open(out, "wb") as f:
    f.write(b"icns" + struct.pack(">I", len(chunks) + 8) + chunks)
PY
echo "  images/neoglogg.icns"

# ---------------------------------------------------------------------------
# In-app toolbar and status glyphs.
#
# Monochrome, one colour per theme. No single colour reaches a 7:1 ratio
# against both the light button background and the dark theme's #353535, so
# each glyph is rendered twice; Theme::iconPath() picks the set at runtime.
#
#   #3C4043 on #EFEFEF -> 9.1:1     #D8D8D8 on #353535 -> 8.6:1
#
# The tab indicators are the one exception to monochrome: they are meant to be
# caught peripherally across a row of tabs, which a grey disc cannot do. Those
# two keep colour, still clearing the 3:1 non-text floor by a wide margin
# (worst case #1A73E8 on #EFEFEF, 3.9:1).
#
# The sources carry the light colours literally so they stay viewable in an
# editor; the dark set is the same file with those values substituted.
LIGHT_FG=3C4043;   DARK_FG=D8D8D8
LIGHT_INFO=1A73E8; DARK_INFO=8AB4F8
LIGHT_ALERT=D93025; DARK_ALERT=F28B82

mkdir -p images/dark
for glyph in images/glyphs/*.svg; do
    name=$(basename "$glyph" .svg)

    sed -e "s/#$LIGHT_FG/#$DARK_FG/gI" \
        -e "s/#$LIGHT_INFO/#$DARK_INFO/gI" \
        -e "s/#$LIGHT_ALERT/#$DARK_ALERT/gI" \
        "$glyph" > "$WORK/$name-dark.svg"

    for scale in 1 2; do
        px=$((24 * scale))
        [ $scale = 1 ] && suffix="" || suffix="@2x"

        inkscape "$glyph" -o "images/${name}${suffix}.png" \
            -w $px -h $px >/dev/null 2>&1
        inkscape "$WORK/$name-dark.svg" -o "images/dark/${name}${suffix}.png" \
            -w $px -h $px >/dev/null 2>&1

        if [ `which optipng` ]; then
            optipng -quiet -o5 "images/${name}${suffix}.png" \
                              "images/dark/${name}${suffix}.png"
        fi
    done
    echo "  glyph $name (light + dark, 24 + @2x)"
done

# macOS .dmg window background. Regenerated here because the checked-in PNG
# had drifted from its source and still read "glogg" long after the rename.
inkscape images/osx_installer.svg -o images/osx_installer.png \
    -w 550 -h 352 >/dev/null 2>&1
inkscape images/osx_installer.svg -o images/osx_installer@2x.png \
    -w 1100 -h 704 >/dev/null 2>&1
if [ `which optipng` ]; then
    optipng -quiet -o5 images/osx_installer.png images/osx_installer@2x.png
fi
echo "  images/osx_installer.png (+ @2x)"

echo "Done."
