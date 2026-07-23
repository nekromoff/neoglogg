neoglogg - the fast, smart log explorer. updated and upgraded.
=====================================

neoglogg by Daniel Duris, based on glogg by Nicolas Bonnefon, is a multi-platform GUI application that helps browse and search through long and complex log files.  It is designed with programmers and system administrators in mind and can be seen as a graphical, interactive combination of grep and less.

## You've been warned

This is a partial update. I needed wrap lines features, but then decided to include the older pull requests when possible. No releases yet, but you can compile. Install and build scripts have been updated. If they don't work for you, open a pull request. I have tested on Linux only.

## Main features

* Runs on Unix-like systems, Windows and Mac thanks to Qt
* Provides a second window showing the result of the current search
* Reads UTF-8 and ISO-8859-1 files
* Supports grep/egrep like regular expressions
* Colorizes the log and search results
* Displays a context view of where in the log the lines of interest are
* Is fast and reads the file directly from disk, without loading it into memory
* Is open source, released under the GPL

## New in neoglogg

* Line wrapping, toggled from the View menu. Long lines are wrapped at the viewport width instead of scrolling horizontally. Designed to stay fast on very long lines (hundreds of KB per line): the wrap layout is computed by simple character splitting and cached, and the view scrolls by visual rows (mouse wheel, arrow and page keys; Home/End jump to the first/last page of the top line). The setting is remembered across sessions.

Merged from upstream glogg pull requests:

* Dark theme option (Fusion palette) ([#301](https://github.com/nickbnf/glogg/pull/301))
* Line-range filtering for searches, plus a decoded-line dock ([#297](https://github.com/nickbnf/glogg/pull/297))
* Multi-language UI support ([#276](https://github.com/nickbnf/glogg/pull/276))
* Opening multiple files at once from the Open dialog ([#272](https://github.com/nickbnf/glogg/pull/272))
* Dynamic highlight colors based on the search pattern ([#241](https://github.com/nickbnf/glogg/pull/241))
* Color picker for filter colors ([#194](https://github.com/nickbnf/glogg/pull/194))
* "Follow file" checkbox in the toolbar ([#51](https://github.com/nickbnf/glogg/pull/51))
* Up/Down keys move the selection like j/k ([#134](https://github.com/nickbnf/glogg/pull/134)), Escape closes the search bar and case-change re-runs the search ([#143](https://github.com/nickbnf/glogg/pull/143))
* Bug fixes and internal cleanups: filtered-view index errors ([#223](https://github.com/nickbnf/glogg/pull/223)), out-of-bounds access ([#254](https://github.com/nickbnf/glogg/pull/254)), worker-thread atomicity ([#220](https://github.com/nickbnf/glogg/pull/220)), eager filtered-lines cache ([#224](https://github.com/nickbnf/glogg/pull/224)), Clang warnings ([#228](https://github.com/nickbnf/glogg/pull/228)), PersistentCopy removal ([#230](https://github.com/nickbnf/glogg/pull/230)), spelling ([#255](https://github.com/nickbnf/glogg/pull/255))

## Download

Installers, binaries and source tarballs are not available yet.

## Requirements

* GCC version 4.8.0 or later (any C++11 compiler should do)
* Qt libraries, version 5.2.0 or later (Qt 5 only — Qt 4 and Qt 6 are not supported)
* Boost "program-options" development libraries
* Markdown HTML processor (optional, to generate HTML documentation)

* **Linux (Debian/Ubuntu):** `qtbase5-dev`, `qt5-qmake`, `qtbase5-dev-tools`,
  `libboost-program-options-dev`; optionally `markdown` for the HTML docs.
  D-Bus support (single-instance mode) only needs Qt5 DBus, included in qtbase.
* **Windows (native MinGW):** Qt 5.x for MinGW and Boost, built as in
  `appveyor.yml` (tested with Qt 5.10.1/mingw53_32 and Boost 1.67):
  `qmake -r BOOST_PATH=%BOOST_ROOT%` then `mingw32-make`. NSIS (plus the
  `neoglogg.nsi` script) is only needed to produce the installer.
  Note: `INSTALL.win.md` and `release-win32-x.sh` describe the legacy Qt4-era
  Linux cross-build and have not been updated or tested — prefer the native
  MinGW build.
* **macOS:** static Qt 5.8+ build and Boost 1.59+ (see `release-osx.sh` for
  the expected paths); `node`/`appdmg` (via Homebrew and npm) only for
  packaging the DMG installer.
* **Tests:** CMake, Qt5 and the Google Mock sources (`GMOCK_HOME`), see below.

neoglogg has been developed and tested on Linux only so far.

## Building

The build system uses qmake (Qt 5). An out-of-tree build in `build/` is
recommended:

```
mkdir -p build
cd build
qmake ../neoglogg.pro
make
make install INSTALL_ROOT=/usr/local (as root if needed)
```

On Debian/Ubuntu the required packages are `qtbase5-dev`, `qt5-qmake`,
`qtbase5-dev-tools` and `libboost-program-options-dev`.

`qmake BOOST_PATH=/path/to/boost/` will statically compile the required parts of
the Boost libraries whose source are found at the specified path.
The path should be the directory where the tarball from www.boost.org is
extracted.
(use this method on Windows or if Boost is not available on the system)

The documentation is built and installed automatically if 'markdown'
is found.

## Tests

The tests are built using CMake, and require Qt5 (5.8 or later for the test
suite) and the Google Mock source (pointed to by `GMOCK_HOME`).

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
