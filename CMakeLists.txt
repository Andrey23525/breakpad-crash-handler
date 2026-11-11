cmake_minimum_required(VERSION 3.21)

project(breakpad_crash_handler
    VERSION 1.0.0
    DESCRIPTION "Out-of-process Breakpad crash handler with a Qt GUI"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Project configuration. These cache variables describe the project and are
# baked into src/project_config.h at configure time; override with -D...=...
# ---------------------------------------------------------------------------
set(CRASH_SUPPORT_EMAIL        "support@example.com"                 CACHE STRING "Support email for the 'send by email' action")
set(CRASH_UPLOAD_URL           "https://crash.example.com/api/upload" CACHE STRING "Developer upload endpoint")
set(CRASH_PRODUCT_NAME         "YourApp"                             CACHE STRING "Default display name of the protected app")
set(CRASH_PIPE_NAME            "crash_handler"                       CACHE STRING "Default named-pipe name (without \\\\.\\pipe\\ prefix)")
set(CRASH_UPLOAD_FIELD_PRODUCT "product"                             CACHE STRING "Multipart field name for the product")
set(CRASH_UPLOAD_FIELD_REPORT  "report"                              CACHE STRING "Multipart field name for the report")
set(CRASH_UPLOAD_FIELD_DUMP    "dump"                                CACHE STRING "Multipart field name for the dump")

configure_file(
    src/project_config.h.in
    ${CMAKE_BINARY_DIR}/project_config.h
    @ONLY
)

# Qt discovery and setup (AUTOMOC/AUTOUIC/AUTORCC + Qt policies).
include(cmake/setup_qt.cmake)

# Build the Breakpad client (crash-generation server) from the submodule.
include(cmake/Breakpad.cmake)
breakpad_add_client(breakpad_client)

# Demo client (test/screenshot tool): registers with the handler over a named
# pipe and crashes on purpose. Not part of the shipped handler.
add_executable(crash_demo_client
    tests/demo_client.cpp
)
target_link_libraries(crash_demo_client PRIVATE breakpad_client)
target_compile_definitions(crash_demo_client PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)

# Example client: demonstrates how a protected app reports crashes over the
# pipe using Breakpad's out-of-process ExceptionHandler.
add_executable(example_client
    examples/client/main.cpp
)
target_link_libraries(example_client PRIVATE breakpad_client)
target_compile_definitions(example_client PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)

qt_add_executable(crash_handler WIN32
    src/main.cpp
    src/crashserver.h
    src/crashserver.cpp
    src/crashreport.h
    src/minidumpreader.h
    src/minidumpreader.cpp
    src/networkmonitor.h
    src/networkmonitor.cpp
    src/senders.h
    src/senders.cpp
    src/stringutils.h
    src/mainwindow.h
    src/mainwindow.cpp
)

# Precompiled header with third-party includes (project codestyle).
target_precompile_headers(crash_handler PRIVATE src/pch.h)

# ${CMAKE_BINARY_DIR} exposes the generated project_config.h.
target_include_directories(crash_handler PRIVATE
    src
    ${CMAKE_BINARY_DIR}
)

target_link_libraries(crash_handler PRIVATE
    ${QT_TARGETS}
    breakpad_client
)

# crashserver.cpp pulls in <windows.h> via the Breakpad server header.
target_compile_definitions(crash_handler PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    UNICODE
    _UNICODE
)

# Qt Linguist translations. The .ts files are compiled to .qm and embedded
# as a resource under ":/translations", so the single crash_handler.exe is
# fully self-contained. Add more .ts files here to ship more languages.
qt_add_translations(crash_handler
    TS_FILES
        src/translations/crash_handler_ru_RU.ts
        src/translations/crash_handler_zh_CN.ts
    RESOURCE_PREFIX "/translations"
    SOURCE_TARGETS crash_handler
)

if(MSVC)
    # /utf-8 is required: sources and string literals are UTF-8.
    target_compile_options(crash_handler PRIVATE /W4 /utf-8)
else()
    target_compile_options(crash_handler PRIVATE -Wall -Wextra)
endif()
