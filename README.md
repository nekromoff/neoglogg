neoglogg - the fast, smart log explorer. updated and upgraded.
=====================================

neoglogg by Daniel Duris, based on glogg by Nicolas Bonnefon, is a multi-platform GUI application that helps browse and search through long and complex log files.  It is designed with programmers and system administrators in mind and can be seen as a graphical, interactive combination of grep and less.

<img width="512" height="512" alt="neoglogg" src="https://github.com/user-attachments/assets/12c419fe-6e20-435f-947e-358daae43581" />

## Main features

* Linux, Windows, Mac OS supported (thanks to Qt)
* Extremely fast - streaming files directly from disk, without loading into memory
* Parallel search across all available processor cores
* UTF-8, UTF-16, CP1251, CP1252, Big5, GB18030, ISO-8859-1, Shift_JIS
  and KOI8-R, automatic detection support
* Regex search (PCRE2, via `QRegularExpression`)
* Fast orientation in the log and search results - highlighted matched lines
* A fast context view - where in the log the lines
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

Deb package, Windows, Mac OS, AppImage

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
* **Tests:** CMake (3.16+), Qt 6 (including Core5Compat and Test) and the
  Google Mock sources (`GMOCK_HOME`), see below.

neoglogg has been developed and tested on Linux only so far.

## Building

The build system uses qmake (Qt 6). An out-of-tree build in `build/` is
recommended:

```
mkdir -p build
cd build
qmake6 ../neoglogg.pro
make
make install INSTALL_ROOT=/usr/local (as root if needed)
```

On Debian/Ubuntu the required packages are `qt6-base-dev`, `qt6-5compat-dev`,
`qmake6` and `libboost-program-options-dev`.

`qmake BOOST_PATH=/path/to/boost/` will statically compile the required parts of
the Boost libraries whose source are found at the specified path.
The path should be the directory where the tarball from www.boost.org is
extracted.
(use this method on Windows or if Boost is not available on the system)

## Tests

The tests are built using CMake (3.16 or later), and require Qt 6 (with the
Core5Compat and Test modules) and the Google Mock source (pointed to by
`GMOCK_HOME`).

```
cd tests
mkdir build
cd build
export QT_DIR=/path/to/qt/if/non/standard
export GMOCK_HOME=/path/to/gmock
cmake ..
make
./neoglogg_tests
```

## Authors and copyright

- Copyright (c) 2009–2018 Nicolas Bonnefon
- Copyright (c) 2026+ Daniel Duris, dusoft@staznosti.sk

Releases
--------

Pushing a version tag builds and publishes every artifact:

```
git tag -a v1.2.3 -m "1.2.3"
git push origin v1.2.3
```

`.github/workflows/release.yml` then produces a `.deb`, an `.AppImage`, a
Windows installer plus portable zip, and a macOS `.dmg`, and attaches them to
a **draft** GitHub Release for review before publishing.

`.github/workflows/ci.yml` compiles on Linux, Windows and macOS for every push
and pull request. It does not run the test suite: `tests/CMakeLists.txt` still
expects a hand-built Google Mock via `$GMOCK_HOME`.

The AppImage is built on Ubuntu 22.04 deliberately — it bundles Qt but still
links the host glibc, which is not forward compatible.

Neither the Windows installer nor the macOS disk image is code-signed, so both
warn on first launch. On macOS, right-click > Open the first time.
