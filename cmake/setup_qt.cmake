# setup_qt.cmake
#
# Qt discovery and setup, mirroring statistics-32's setup_qt.cmake.

cmake_minimum_required(VERSION 3.15)

set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL "Release")
set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO "RelWithDebInfo;Release;")

# Qt location; override with the Qt6_ROOT environment variable if needed.
set(Qt6_ROOT "C:/Work/libraries/Qt/6.10.2/msvc2022_64/")
if(DEFINED ENV{Qt6_ROOT})
    set(Qt6_ROOT $ENV{Qt6_ROOT})
endif()

message(STATUS "Qt6_ROOT ${Qt6_ROOT}")

set(QT_COMPONENTS Core Gui Widgets Network)
set(QT_TARGETS "")
foreach(comp ${QT_COMPONENTS})
    list(APPEND QT_TARGETS "Qt6::${comp}")
endforeach()

set(QT_MIN_VERSION 6.9)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS ${QT_COMPONENTS})

# Enables AUTOMOC/AUTOUIC/AUTORCC and Qt policies.
qt_standard_project_setup(REQUIRES ${QT_MIN_VERSION})

find_package(Qt6 REQUIRED COMPONENTS ${QT_COMPONENTS} LinguistTools)
