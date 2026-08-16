# -------------------------------------------------
# neoglogg
# -------------------------------------------------

# Debug builds: qmake CONFIG+=debug
# Release builds: qmake

TARGET = neoglogg
TEMPLATE = app

QT += core widgets network

# QTextCodec (used for the many supported encodings) moved to the
# Core5Compat module in Qt 6. QStringConverter is not a replacement:
# it does not know CP1251, Big5, GB18030, Shift_JIS or KOI8-R.
QT += core5compat

win32:Debug:CONFIG += console

# Necessary when cross-compiling:
win32:Release:QMAKE_LFLAGS += "-Wl,-subsystem,windows"

# Input
SOURCES += \
    src/main.cpp \
    src/session.cpp \
    src/data/abstractlogdata.cpp \
    src/data/logdata.cpp \
    src/data/logfiltereddata.cpp \
    src/data/logfiltereddataworkerthread.cpp \
    src/data/logdataworkerthread.cpp \
    src/data/compressedlinestorage.cpp \
    src/mainwindow.cpp \
    src/crawlerwidget.cpp \
    src/abstractlogview.cpp \
    src/logmainview.cpp \
    src/filteredview.cpp \
    src/optionsdialog.cpp \
    src/persistentinfo.cpp \
    src/configuration.cpp \
    src/filtersdialog.cpp \
    src/filterset.cpp \
    src/savedsearches.cpp \
    src/infoline.cpp \
    src/menuactiontooltipbehavior.cpp \
    src/selection.cpp \
    src/quickfind.cpp \
    src/quickfindpattern.cpp \
    src/quickfindwidget.cpp \
    src/sessioninfo.cpp \
    src/recentfiles.cpp \
    src/overview.cpp \
    src/overviewwidget.cpp \
    src/marks.cpp \
    src/quickfindmux.cpp \
    src/signalmux.cpp \
    src/tabbedcrawlerwidget.cpp \
    src/viewtools.cpp \
    src/encodingspeculator.cpp \
    src/theme.cpp \
    src/neogloggapp.cpp \

INCLUDEPATH += src/

HEADERS += \
    src/data/abstractlogdata.h \
    src/data/logdata.h \
    src/data/logfiltereddata.h \
    src/data/logfiltereddataworkerthread.h \
    src/data/logdataworkerthread.h \
    src/data/threadprivatestore.h \
    src/data/compressedlinestorage.h \
    src/data/linepositionarray.h \
    src/mainwindow.h \
    src/session.h \
    src/viewinterface.h \
    src/crawlerwidget.h \
    src/logmainview.h \
    src/log.h \
    src/filteredview.h \
    src/abstractlogview.h \
    src/optionsdialog.h \
    src/persistentinfo.h \
    src/configuration.h \
    src/filtersdialog.h \
    src/filterset.h \
    src/savedsearches.h \
    src/infoline.h \
    src/filewatcher.h \
    src/selection.h \
    src/quickfind.h \
    src/quickfindpattern.h \
    src/quickfindwidget.h \
    src/sessioninfo.h \
    src/persistable.h \
    src/recentfiles.h \
    src/menuactiontooltipbehavior.h \
    src/overview.h \
    src/overviewwidget.h \
    src/marks.h \
    src/qfnotifications.h \
    src/quickfindmux.h \
    src/signalmux.h \
    src/tabbedcrawlerwidget.h \
    src/loadingstatus.h \
    src/externalcom.h \
    src/viewtools.h \
    src/encodingspeculator.h \
    src/theme.h \
    src/neogloggapp.h \

isEmpty(BOOST_PATH) {
    message(Building using system dynamic Boost libraries)
    macx {
      # Path for brew installed libs
      INCLUDEPATH += /usr/local/include
      LIBS += -L/usr/local/lib -lboost_program_options-mt
    }
    else {
      LIBS += -lboost_program_options
    }
}
else {
    message(Building using static Boost libraries at $$BOOST_PATH)

    SOURCES += $$BOOST_PATH/libs/program_options/src/*.cpp

    exists( $$BOOST_PATH/libs/smart_ptr/src/sp_collector.cpp ) {
        message( "'old' version of Boost" )
        SOURCES += $$BOOST_PATH/libs/smart_ptr/src/*.cpp
    }
    else {
        message( "'new' version of Boost" )
        SOURCES += $$BOOST_PATH/libs/smart_ptr/extras/src/*.cpp
    }

    INCLUDEPATH += $$BOOST_PATH
}

FORMS += src/optionsdialog.ui
FORMS += src/filtersdialog.ui

macx {
    # Icon for Mac
    ICON = images/neoglogg.icns
}
else {
    # For Windows icon
    RC_ICONS = neoglogg.ico
    QMAKE_TARGET_COMPANY = "Nicolas Bonnefon"
    QMAKE_TARGET_DESCRIPTION = "neoglogg - the fast, smart log explorer. Updated and upgraded."
}

RESOURCES = neoglogg.qrc

# Install (for unix)
icon64.path  = $$PREFIX/share/icons/hicolor/64x64/apps
icon64.files = images/hicolor/64x64/neoglogg.png

icon128.path  = $$PREFIX/share/icons/hicolor/128x128/apps
icon128.files = images/hicolor/128x128/neoglogg.png

icon256.path  = $$PREFIX/share/icons/hicolor/256x256/apps
icon256.files = images/hicolor/256x256/neoglogg.png

icon_svg.path  = $$PREFIX/share/icons/hicolor/scalable/apps
icon_svg.files = images/hicolor/scalable/neoglogg.svg

doc.path  = $$PREFIX/share/doc/neoglogg
doc.files += README.md LICENSE

desktop.path = $$PREFIX/share/applications
desktop.files = neoglogg.desktop

target.path = $$PREFIX/bin
INSTALLS = target icon64 icon128 icon256 icon_svg doc desktop

# Build directories
CONFIG(debug, debug|release) {
    DESTDIR = debug
} else {
    DESTDIR = release
}

OBJECTS_DIR = $${OUT_PWD}/.obj/$${DESTDIR}-shared
MOC_DIR = $${OUT_PWD}/.moc/$${DESTDIR}-shared
UI_DIR = $${OUT_PWD}/.ui/$${DESTDIR}-shared

# Debug symbols even in release build
QMAKE_CXXFLAGS = -g

# Qt 6 requires C++17
CONFIG += c++17

# Extra compiler arguments
# QMAKE_CXXFLAGS += -Weffc++
QMAKE_CXXFLAGS += -Wextra

GPROF {
    QMAKE_CXXFLAGS += -pg
    QMAKE_LFLAGS   += -pg
}

isEmpty(LOG_LEVEL) {
    CONFIG(debug, debug|release) {
        DEFINES += FILELOG_MAX_LEVEL=\"logDEBUG4\"
    } else {
        DEFINES += FILELOG_MAX_LEVEL=\"logDEBUG\"
    }
}
else {
    message("Using specified log level: $$LOG_LEVEL")
    DEFINES += FILELOG_MAX_LEVEL=\"$$LOG_LEVEL\"
}

macx {
    QMAKE_CXXFLAGS += -stdlib=libc++
    QMAKE_LFLAGS += -stdlib=libc++

    # Qt 6 supports macOS 11 and later
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 11.0

    QMAKE_INFO_PLIST = Info.plist
}

# Official builds can be generated with `qmake VERSION="1.2.3"'
isEmpty(VERSION):system(date >/dev/null) {
    system([ -f .tarball-version ]) {
        QMAKE_CXXFLAGS += -DNEOGLOGG_VERSION=\\\"`cat .tarball-version`\\\"
    }
    else {
        QMAKE_CXXFLAGS += -DNEOGLOGG_DATE=\\\"`date +'\"%F\"'`\\\"
        QMAKE_CXXFLAGS += -DNEOGLOGG_VERSION=\\\"`git describe`\\\"
        QMAKE_CXXFLAGS += -DNEOGLOGG_COMMIT=\\\"`git rev-parse --short HEAD`\\\"
    }
}
else {
    QMAKE_CXXFLAGS += -DNEOGLOGG_VERSION=\\\"$$VERSION\\\"
}

# Optional features (e.g. CONFIG+=no-dbus)
# qtHaveModule asks the Qt actually being built against, rather than
# pkg-config, which probed a fixed Qt version regardless of the Qt in use.
qtHaveModule(dbus):!no-dbus {
    message("Support for D-BUS will be included")
    QT += dbus
    QMAKE_CXXFLAGS += -DNEOGLOGG_SUPPORTS_DBUS
    SOURCES += src/dbusexternalcom.cpp
    HEADERS += src/dbusexternalcom.h
}
else {
    message("Support for D-BUS will NOT be included")
    win32 | macx {
        message("Support for cross-platform IPC will be included")
        QMAKE_CXXFLAGS += -DNEOGLOGG_SUPPORTS_SOCKETIPC
        SOURCES += src/socketexternalcom.cpp
        HEADERS += src/socketexternalcom.h
    }
}

# Version checking
version_checker {
    message("Version checker will be included")
    QT += network
    QMAKE_CXXFLAGS += -DNEOGLOGG_SUPPORTS_VERSION_CHECKING
    SOURCES += src/versionchecker.cpp
    HEADERS += src/versionchecker.h
}
else {
    message("Version checker will NOT be included")
}

# File watching
linux-g++ | linux-g++-64 {
    CONFIG += inotify
}

macx {
    CONFIG += kqueue
}

win32 {
    message("File watching using Windows")
    SOURCES += src/platformfilewatcher.cpp src/winwatchtowerdriver.cpp src/watchtower.cpp src/watchtowerlist.cpp
    HEADERS += src/platformfilewatcher.h src/winwatchtowerdriver.h src/watchtower.h src/watchtowerlist.h
    QMAKE_CXXFLAGS += -DNEOGLOGG_SUPPORTS_POLLING
}
else {
    inotify {
        message("File watching using inotify")
        QMAKE_CXXFLAGS += -DNEOGLOGG_SUPPORTS_INOTIFY
        SOURCES += src/platformfilewatcher.cpp src/inotifywatchtowerdriver.cpp src/watchtower.cpp src/watchtowerlist.cpp
        HEADERS += src/platformfilewatcher.h src/inotifywatchtowerdriver.h src/watchtower.h src/watchtowerlist.h
    }
    else {
        macx {
            message("File watching using kqueue")
            QMAKE_CXXFLAGS += -DNEOGLOGG_SUPPORTS_KQUEUE
            SOURCES += src/platformfilewatcher.cpp src/kqueuewatchtowerdriver.cpp src/watchtower.cpp src/watchtowerlist.cpp
            HEADERS += src/platformfilewatcher.h src/kqueuewatchtowerdriver.h src/watchtower.h src/watchtowerlist.h
        }
        else {
            message("File watching using Qt")
            SOURCES += src/qtfilewatcher.cpp
            HEADERS += src/qtfilewatcher.h
        }
    }
}

# Performance measurement
perf {
    QMAKE_CXXFLAGS += -DNEOGLOGG_PERF_MEASURE_FPS
}
