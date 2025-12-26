cmake_minimum_required(VERSION 3.16)
project(LinuxTrack VERSION 1.0.6 LANGUAGES C CXX)

# Default installation prefix
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/opt/linuxtrack" CACHE PATH "Default install path" FORCE)
endif()

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# Options
option(BUILD_GUI "Build Qt GUI" ON)
option(BUILD_MICKEY "Build Mickey mouse emulator" ON)
option(BUILD_WINE_BRIDGE "Build Wine bridge (requires winegcc)" ON)

# Project version (legacy compatibility)
set(PACKAGE_VERSION "${PROJECT_VERSION}")

# Include modules for feature testing
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckTypeSize)
include(GNUInstallDirs)
include(FindPkgConfig)

# Dependency Checks
pkg_check_modules(MXML REQUIRED mxml)
pkg_check_modules(LIBUSB10 libusb-1.0)
pkg_check_modules(LIBLO liblo)
pkg_check_modules(OPENCV opencv4)

find_library(LTR_LIBM m)
find_library(LTR_LIBDL dl)
find_library(LTR_LIBPTHREAD pthread)
find_library(LTR_LIBV4L2 v4l2)

# Qt Dependencies for GUI and Mickey
if(BUILD_GUI OR BUILD_MICKEY)
    find_package(Qt6 COMPONENTS Widgets Gui Network Help OpenGL OpenGLWidgets REQUIRED)
endif()

# X11 for Mickey
if(BUILD_MICKEY)
    find_package(X11 REQUIRED)
endif()

# GLU for GUI
if(BUILD_GUI)
    find_package(OpenGL REQUIRED)
    find_library(GLU_LIBRARY GLU)
endif()

find_package(ZLIB REQUIRED)
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# Paths for installation and configuration
set(Linuxtrack_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/linuxtrack")
set(Linuxtrack_HELP_DIR "${Linuxtrack_DATA_DIR}/help")
add_definitions(-DLTR_DATA_PATH="${Linuxtrack_DATA_DIR}")

# System Check: Headers
check_include_file(assert.h HAVE_ASSERT_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(netdb.h HAVE_NETDB_H)
check_include_file(stdbool.h HAVE_STDBOOL_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdio.h HAVE_STDIO_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_file(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h HAVE_UNISTD_H)

# System Check: Functions
check_function_exists(atexit HAVE_ATEXIT)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(memset HAVE_MEMSET)
check_function_exists(select HAVE_SELECT)
check_function_exists(socket HAVE_SOCKET)
check_function_exists(strerror HAVE_STRERROR)

# Platform specifics
if(APPLE)
    set(HAS_DARWIN 1)
    add_definitions(-DAPL=1 -DIBM=0 -DLIN=0)
else()
    set(HAS_LINUX 1)
    add_definitions(-DAPL=0 -DIBM=0 -DLIN=1)
    set(HAS_V4L2 1)
endif()

# Configuration variables for config.h
set(PACKAGE_BUGREPORT "https://github.com/uglyDwarf/linuxtrack/issues")
set(PACKAGE_NAME "LinuxTrack")
set(PACKAGE_TARNAME "linuxtrack")
set(PACKAGE_URL "")

# Generate headers
configure_file(config.h.cmake ${CMAKE_BINARY_DIR}/config.h)
configure_file(src/pathconfig.h.cmake ${CMAKE_BINARY_DIR}/src/pathconfig.h)

# Global compiler settings
add_definitions(-DHAVE_CONFIG_H)
if(NOT APPLE)
    add_definitions(-DLIB_PATH="${CMAKE_INSTALL_PREFIX}/lib/linuxtrack/")
endif()

include_directories(${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/src)

# RPATH settings for installation
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/linuxtrack:${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Wait, we want $ORIGIN based RPATH for relocatability sometimes, 
# but for /opt/linuxtrack absolute paths are fine too since that's the fixed prefix.
# However, to be modern:
file(RELATIVE_PATH REL_LIB_PATH "${CMAKE_INSTALL_PREFIX}/bin" "${CMAKE_INSTALL_PREFIX}/lib/linuxtrack")
list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN/${REL_LIB_PATH}")
file(RELATIVE_PATH REL_LIB_PATH2 "${CMAKE_INSTALL_PREFIX}/bin" "${CMAKE_INSTALL_PREFIX}/lib")
list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN/${REL_LIB_PATH2}")

# Subdirectories
add_subdirectory(src)

if(BUILD_GUI)
    add_subdirectory(src/qt_gui)
endif()

if(BUILD_MICKEY)
    add_subdirectory(src/mickey)
endif()

if(BUILD_WINE_BRIDGE)
    find_program(WINEGCC winegcc)
    find_program(WINEGXX wineg++)
    find_program(WRC wrc)
    find_program(MAKENSIS makensis)
    if(WINEGCC AND WINEGXX AND WRC)
        message(STATUS "Found Wine build tools - building Wine bridge")
        add_subdirectory(src/wine_bridge)
    else()
        message(WARNING "Wine build tools not found - skipping Wine bridge")
    endif()
endif()

# Installation of global files
install(FILES README README.devel README.xplane doc/README.ltr_pipe DESTINATION share/doc/linuxtrack)
install(FILES src/linuxtrack.desktop src/linuxtrack-wii.desktop DESTINATION share/applications)
install(FILES src/linuxtrack.svg src/linuxtrack.png src/linuxtrack.xpm 
              src/linuxtrack-wii.svg src/linuxtrack-wii.png src/linuxtrack-wii.xpm
        DESTINATION share/pixmaps)
