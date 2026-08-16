neoglogg - the fast, smart log explorer. Updated and upgraded.
=====================================

neoglogg by Daniel Duris, based on glogg, is a multi-platform GUI log viewer.

Browse and search through long and complex log files. Also described as: A graphical, interactive combination of grep and less.

<img width="512" height="512" alt="neoglogg" src="https://github.com/user-attachments/assets/12c419fe-6e20-435f-947e-358daae43581" />

## Main features

* Linux, Windows, Mac OS supported (thanks to Qt)
* Extremely fast - streaming files directly from disk, without loading into memory
* Parallel search across all available processor cores
* UTF-8, UTF-16, CP1251, CP1252, Big5, GB18030, ISO-8859-1, Shift_JIS
  and KOI8-R, automatic detection support
* Regex search (PCRE2, via `QRegularExpression`)
* Fast orientation in the log and search results - highlighted matched lines
* Follow file mode
* Line wrapping for long lines
* Light / dark mode

## New in neoglogg

**Upgraded to Qt 6** The whole application, build system and test suite have
been ported from Qt 5 to Qt 6 (using the Core5Compat module for the extra
text encodings).

**Line wrapping**, toggled from the View menu. Long lines are wrapped at the
viewport width instead of scrolling horizontally. Designed to stay fast on very
long lines (hundreds of KB per line): the wrap layout is computed by simple
character splitting and cached, and the view scrolls by visual rows (mouse
wheel, arrow and page keys; Home/End jump to the first/last page of the top
line). The setting is remembered across sessions.

**Multi file opening** Several files can be opened at once from the Open dialog (Shift/Ctrl-select),
each in its own tab.

**Parallel search** Searches are spread across the machine's cores instead of
running on a single thread. The file is cut into fixed-size chunks, a batch of
them is searched concurrently, and the results are merged back in file order so
the match list stays sorted. Cancelling a search commits only the chunks that
actually finished, so a stopped-and-resumed search cannot skip lines. The worker
count is configurable under Options → General → Search options; the default,
"Automatic", uses one worker per core.

**Dark mode** An application-wide dark colour scheme (Fusion style plus a dark
palette), toggled under Options → General → Appearance and remembered across
sessions. The log views take their colours from the palette, so they follow the
theme rather than staying white.

**New identity and icons** Icons and installation images for all systems (Linux, Windows, Mac OS)

## Installation

Linux (DEB, RPM, AUR packages), Windows, Mac OS, AppImage

See Releases: https://github.com/nekromoff/neoglogg/releases/

## Requirements

* A C++17 compiler (GCC 8 or later, Clang 8 or later)
* Qt 6 libraries, including the Core5Compat module (needed for `QTextCodec`,
  which covers the encodings `QStringConverter` does not: CP1251, Big5,
  GB18030, Shift_JIS, KOI8-R). Qt 5 is no longer supported.
* Boost "program-options" development libraries

* **Linux (Debian/Ubuntu):** `qt6-base-dev`, `qt6-base-dev-tools`,
  `qt6-5compat-dev`, `libboost-program-options-dev`.
  D-Bus support (single-instance mode) only needs Qt6 DBus, included in
  qt6-base.
* **Windows (native MinGW):** MSYS2 with `mingw-w64-x86_64-qt6-base`,
  `mingw-w64-x86_64-qt6-5compat` and `mingw-w64-x86_64-boost`, as in
  `.github/workflows/ci.yml`. NSIS (plus the `neoglogg.nsi` script) is only
  needed to produce the installer.
* **macOS:** Qt 6 and the Boost sources. The release workflow compiles
  `program_options` from source via `BOOST_PATH` and packages with
  `macdeployqt -dmg`, so neither Homebrew Boost nor `node`/`appdmg` is
  required; `release-osx.sh` remains for the older local flow.
* **Tests:** CMake (3.16+) and Qt 6 (including Core5Compat and Test).
  GoogleTest is downloaded automatically at configure time.

Development happens on Linux; CI compiles Linux, Windows and macOS, but only
Linux is tested by hand.

## Building

The build system uses qmake (Qt 6). An out-of-tree build in `build/` is
recommended:

```
mkdir -p build
cd build
qmake6 ../neoglogg.pro PREFIX=/usr
make
sudo make install
```

`PREFIX` is where the install rules point (binary, `.desktop`, icons, docs);
it defaults to empty, so passing it is what puts the binary in `/usr/bin`
rather than `/bin`. `INSTALL_ROOT` is separate and prepends a staging
directory without changing the recorded paths, which is what the packaging
scripts use:

```
make install INSTALL_ROOT=/path/to/staging
```

See the Requirements section above for the packages needed.

`qmake BOOST_PATH=/path/to/boost/` will statically compile the required parts of
the Boost libraries whose source are found at the specified path.
The path should be the directory where the tarball from www.boost.org is
extracted.
(use this method on Windows or if Boost is not available on the system)

## Tests

The tests are built using CMake (3.16 or later) and require Qt 6 (with the
Core5Compat and Test modules). GoogleTest is fetched at configure time, so
there is nothing to install or point at first — only a network connection on
the first configure.

```
cmake -S tests -B tests/build
cmake --build tests/build --target neoglogg_tests -j$(nproc)
QT_QPA_PLATFORM=offscreen ./tests/build/neoglogg_tests
```

The WatchTower tests create files under `/tmp` to trigger real inotify
events, so they need a writable temporary directory.

## Releases

Pushing a version tag builds and publishes every artifact:

```
git tag -a v1.2.3 -m "1.2.3"
git push origin v1.2.3
```

`.github/workflows/release.yml` then produces a `.deb`, an `.rpm`, an
`.AppImage`, a Windows installer plus portable zip, and a macOS `.dmg`, and
attaches them to a **draft** GitHub Release for review before publishing.

The RPM is built in a Fedora container from `neoglogg.spec` (the Ubuntu
runners have neither the RPM macros nor Qt 6 under its Fedora package names)
and installs with `dnf`, `yum` or `zypper`.

The AUR package is pushed after the release is published, since its PKGBUILD
checksums the tag's source tarball. It needs an `AUR_SSH_KEY` secret holding
a private key whose public half is registered on the AUR account; without
that secret the job skips itself instead of failing the release. The
PKGBUILD lives in `packaging/aur/` and its `pkgver`, `pkgrel` and checksum
are rewritten from the tag, so edit the rest of it there, not on the AUR.

`.github/workflows/ci.yml` compiles on Linux, Windows and macOS. It runs on
tags and on manual dispatch from the Actions tab, not on ordinary pushes, so
doc-only commits do not trigger a three-platform build. The Linux job also builds and runs the test
suite.

The AppImage is built on Ubuntu 24.04, because Qt 6's 5compat module (needed
for the legacy text codecs) is not packaged before then. An AppImage bundles
Qt but still links the host glibc, which is not forward compatible, so the
result needs glibc 2.39 or newer. Building on an older base would mean
installing Qt through aqtinstall rather than apt.

Neither the Windows installer nor the macOS disk image is code-signed, so both
warn on first launch. On macOS, right-click > Open the first time.

To build an AppImage locally:

```
VERSION=1.2.3 ./release-appimage.sh
```

## Icons

Every icon is generated from a single source, `images/hicolor/scalable/neoglogg.svg`,
plus the monochrome glyph sources in `images/glyphs/`. After editing either, run:

```
./make-icons.sh
```

That regenerates the hicolor PNGs, the Qt resource glyphs (light and dark, 1x
and 2x), `neoglogg.ico`, `images/neoglogg.icns` with its iconset, and the macOS
disk image background. Do not hand-edit the generated files — they are
overwritten on the next run. Requires Inkscape and ImageMagick.

## Authors and copyright

- Copyright (c) 2026+ Daniel Duris, dusoft@staznosti.sk
- Copyright (c) 2009–2018 Nicolas Bonnefon

Computer-assisted development was used in the process.

