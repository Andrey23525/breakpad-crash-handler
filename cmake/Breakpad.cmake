# Breakpad.cmake
#
# Builds the Google Breakpad *client* library used by this crash handler from
# the git submodule at 3rdparty/breakpad. Breakpad ships no CMake build of its
# own (it uses autotools/gyp), so we compile the exact Windows source subset we
# need directly.
#
# The crash handler is the *server* side of Breakpad's out-of-process crash
# reporting: it only needs CrashGenerationServer + MinidumpGenerator, which
# write the minidump themselves via MiniDumpWriteDump (loaded dynamically from
# dbghelp.dll). It does NOT need ExceptionHandler (the in-process client) and
# does NOT need the symbol-processing library — minidump inspection is done in
# this project with a lightweight reader in src/minidumpreader.cpp.

function(breakpad_add_client target)
    set(BP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/breakpad/src")

    if(NOT EXISTS "${BP_ROOT}/client/windows/crash_generation/crash_generation_server.cc")
        message(FATAL_ERROR
            "Breakpad submodule is not checked out. Run:\n"
            "    git submodule update --init --recursive")
    endif()

    set(BP_CLIENT_SOURCES
        ${BP_ROOT}/client/windows/crash_generation/client_info.cc
        ${BP_ROOT}/client/windows/crash_generation/crash_generation_server.cc
        ${BP_ROOT}/client/windows/crash_generation/minidump_generator.cc
        ${BP_ROOT}/common/windows/guid_string.cc
        ${BP_ROOT}/common/windows/string_utils.cc
    )

    add_library(${target} STATIC ${BP_CLIENT_SOURCES})

    # Breakpad's main branch uses C++20 features (std::erase_if, designated
    # initializers); pin it there rather than the project-wide C++23 default.
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )

    target_include_directories(${target} PUBLIC ${BP_ROOT})

    # Breakpad includes <windows.h>; avoid the min/max macro clashes and keep
    # the headers lean.
    target_compile_definitions(${target} PRIVATE
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )

    if(MSVC)
        target_compile_options(${target} PRIVATE /W3 /utf-8)
    else()
        # This project targets Windows for the crash handler; other platforms
        # would need the matching Breakpad source subset instead.
        target_compile_options(${target} PRIVATE -w)
    endif()
endfunction()
