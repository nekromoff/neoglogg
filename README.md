neoglogg - the fast, smart log explorer. updated and upgraded.
=====================================

neoglogg by Daniel Duris, based on glogg by Nicolas Bonnefon, is a multi-platform GUI application that helps browse and search through long and complex log files.  It is designed with programmers and system administrators in mind and can be seen as a graphical, interactive combination of grep and less.

## Main features

* Runs on Unix-like systems, Windows and Mac thanks to Qt
* Provides a second window showing the result of the current search
* Searches in parallel across all available processor cores
* Reads ISO-8859-1, UTF-8, UTF-16, CP1251, CP1252, Big5, GB18030, Shift_JIS
  and KOI8-R, with automatic detection of the common ones
* Supports Perl-compatible regular expressions (PCRE2, via `QRegularExpression`)
* Colorizes the log and search results
* Displays a context view of where in the log the lines of interest are
* Is fast and reads the file directly from disk, without loading it into memory
* Has a dark mode
* Is open source, released under the GPL

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



## Download

Installers, binaries and source tarballs are not available yet.

## Requirements

* A C++17 compiler (GCC 8 or later, Clang 8 or later)
* Qt 6 libraries, including the Core5Compat module (needed for `QTextCodec`,
  which covers the encodings `QStringConverter` does not: CP1251, Big5,
  GB18030, Shift_JIS, KOI8-R). Qt 5 is no longer supported.
* Boost "program-options" development libraries

* **Linux (Debian/Ubuntu):** `qt6-base-dev`, `qt6-5compat-dev`, `qmake6`,
  `libboost-program-options-dev`; optionally `markdown` for the HTML docs.
  D-Bus support (single-instance mode) only needs Qt6 DBus, included in
  qt6-base.
* **Windows (native MinGW):** Qt 6.x for MinGW and Boost, as in
  `appveyor.yml`: `qmake6 -r BOOST_PATH=%BOOST_ROOT%` then `mingw32-make`.
  NSIS (plus the `neoglogg.nsi` script) is only needed to produce the
  installer. Untested since the Qt 6 port.
* **macOS:** static Qt 6 build and Boost (see `release-osx.sh` for the
  expected paths — untested since the Qt 6 port);
  `node`/`appdmg` (via Homebrew and npm) only for packaging the DMG installer.
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
