INCLUDE (CheckSymbolExists)
INCLUDE (CheckCXXSourceCompiles)
INCLUDE (CheckCXXCompilerFlag)

cmake_minimum_required(VERSION 3.16)
project(neoglogg_tests)

# Qt

find_program(QT_QMAKE_EXECUTABLE NAMES qmake6 qmake PATHS $ENV{QT_DIR}/bin)
# Core5Compat provides QTextCodec, used by the log data classes
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test Core5Compat)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

check_cxx_compiler_flag(-Wall HAS_WALL)

if(HAS_WALL)
    set(PARANOID_FLAGS "${PARANOID_FLAGS} -Wall")
endif(HAS_WALL)

# Compiler (C++17 required by Qt 6 and the main build)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_REQUIRED_FLAGS "-std=c++17")
check_cxx_source_compiles("
    #include <memory>
    int main(int, int)
    {
        auto test = std::make_unique<void>();
        return 0;
    }
" HAVE_MAKE_UNIQUE)

check_cxx_source_compiles("
    #include <memory>
    struct Base {
        virtual void func() {}
    };
    struct Derived : public Base {
        void func() override {}
    }
    int main(int, int)
    {
        return 0;
    }
" HAVE_OVERRIDE)

configure_file(../config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# Configuration
check_symbol_exists( symlink "unistd.h" HAVE_SYMLINK )

# find_library(WS2_LIBRARY NAMES ws2_32)

# Libs

set(LIBS gmock gtest)

# Setup testing
enable_testing()
include_directories(
    $ENV{GMOCK_HOME}/include
    $ENV{GMOCK_HOME}/gtest/include
    $ENV{BOOST_ROOT}/
    ../src/
)
link_directories($ENV{GMOCK_HOME}/mybuild $ENV{GMOCK_HOME}/mybuild/gtest)

# Sources
set(neoglogg_SOURCES
    ../src/session.cpp
    ../src/data/abstractlogdata.cpp
    ../src/data/logdata.cpp
    ../src/data/logfiltereddata.cpp
    ../src/data/logfiltereddataworkerthread.cpp
    ../src/data/logdataworkerthread.cpp
    ../src/data/compressedlinestorage.cpp
    ../src/mainwindow.cpp
    ../src/crawlerwidget.cpp
    ../src/abstractlogview.cpp
    ../src/logmainview.cpp
    ../src/filteredview.cpp
    ../src/optionsdialog.cpp
    ../src/persistentinfo.cpp
    ../src/configuration.cpp
    ../src/filtersdialog.cpp
    ../src/filterset.cpp
    ../src/savedsearches.cpp
    ../src/infoline.cpp
    ../src/menuactiontooltipbehavior.cpp
    ../src/selection.cpp
    ../src/quickfind.cpp
    ../src/quickfindpattern.cpp
    ../src/quickfindwidget.cpp
    ../src/sessioninfo.cpp
    ../src/recentfiles.cpp
    ../src/overview.cpp
    ../src/overviewwidget.cpp
    ../src/marks.cpp
    ../src/quickfindmux.cpp
    ../src/signalmux.cpp
    ../src/tabbedcrawlerwidget.cpp
    ../src/watchtowerlist.cpp
    ../src/watchtower.cpp
    ../src/viewtools.cpp
    ../src/encodingspeculator.cpp
    ../src/platformfilewatcher.cpp
    ../src/filewatcher.cpp
)

set(neoglogg_HEADERS
    ../src/mainwindow.h
    ../src/marks.h
    ../src/platformfilewatcher.h
    ../src/filewatcher.h
)

# Unit tests
set(neoglogg_UTESTS
    watchtowerTest.cpp
    linepositionarrayTest.cpp
    encodingspeculatorTest.cpp
)

# Integration tests
set(neoglogg_ITESTS
    logdataTest.cpp
    logfiltereddataTest.cpp
)

# Performance tests
set(neoglogg_PTESTS
    logdataPerfTest.cpp
    logfiltereddataPerfTest.cpp
)


# Options
if (WIN32)
    set(FileWatcherEngine_SOURCES
        ../src/winwatchtowerdriver.cpp
    )
elseif (LINUX)
    # set(FileWatcherEngine_SOURCES ../src/inotifywatchtowerdriver.cpp)
elseif (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNEOGLOGG_SUPPORTS_KQUEUE")
    set(FileWatcherEngine_SOURCES
         ../src/kqueuewatchtowerdriver.cpp
    )
endif (WIN32)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PARANOID_FLAGS} -fPIC -DNEOGLOGG_VERSION=\\\"unit_tests\\\" -g -gdwarf-2")

# Add test cpp file
add_executable(neoglogg_tests
    ${neoglogg_SOURCES}
    ${FileWatcherEngine_SOURCES}
    ${neoglogg_UTESTS}
)

# Link test executable against gtest & gtest_main
target_link_libraries(neoglogg_tests ${LIBS} gtest_main pthread Qt6::Widgets Qt6::Core5Compat)

add_executable(neoglogg_itests
    ${neoglogg_SOURCES}
    ${FileWatcherEngine_SOURCES}
    ${neoglogg_ITESTS}
    itests.cpp
)

target_link_libraries(neoglogg_itests ${LIBS} pthread Qt6::Widgets Qt6::Core5Compat Qt6::Test)

add_executable(neoglogg_ptests
    ${neoglogg_SOURCES}
    ${FileWatcherEngine_SOURCES}
    ${neoglogg_PTESTS}
    itests.cpp
)

target_link_libraries(neoglogg_ptests ${LIBS} pthread Qt6::Widgets Qt6::Core5Compat Qt6::Test)

add_test(
    NAME neoglogg_tests
    COMMAND neoglogg_tests
)
