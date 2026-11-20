# Breakpad.cmake
#
# Builds the Google Breakpad *client* library used by this crash handler from
# the git submodule at 3rdparty/breakpad. Breakpad ships no CMake build of its
# own (it uses autotools/gyp), so we compile the exact Windows source subset we
# need directly.
#
# The crash handler itself is the *server* side of Breakpad's out-of-process
# crash reporting: it needs CrashGenerationServer + MinidumpGenerator, which
# write the minidump themselves via MiniDumpWriteDump (loaded dynamically from
# dbghelp.dll). The library also compiles the client-side pieces
# (CrashGenerationClient + ExceptionHandler) so the demo/example clients can
# link it. The symbol-processing library is not needed — minidump inspection is
# done with a lightweight reader in src/minidumpreader.cpp.

function(breakpad_add_client target)
    set(BP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/breakpad/src")

    if(NOT EXISTS "${BP_ROOT}/client/windows/crash_generation/crash_generation_server.cc")
        message(FATAL_ERROR
            "Breakpad submodule is not checked out. Run:\n"
            "    git submodule update --init --recursive")
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        if(NOT EXISTS "${BP_ROOT}/third_party/lss/linux_syscall_support.h")
            message(FATAL_ERROR "Missing Linux syscall support. See Linux setup in README.md")
        endif()
        find_package(Threads REQUIRED)
        set(BP_CLIENT_SOURCES
            ${BP_ROOT}/client/linux/crash_generation/crash_generation_client.cc
            ${BP_ROOT}/client/linux/crash_generation/crash_generation_server.cc
            ${BP_ROOT}/client/linux/dump_writer_common/thread_info.cc
            ${BP_ROOT}/client/linux/dump_writer_common/ucontext_reader.cc
            ${BP_ROOT}/client/linux/handler/exception_handler.cc
            ${BP_ROOT}/client/linux/handler/minidump_descriptor.cc
            ${BP_ROOT}/client/linux/log/log.cc
            ${BP_ROOT}/client/linux/microdump_writer/microdump_writer.cc
            ${BP_ROOT}/client/linux/minidump_writer/linux_core_dumper.cc
            ${BP_ROOT}/client/linux/minidump_writer/linux_dumper.cc
            ${BP_ROOT}/client/linux/minidump_writer/linux_ptrace_dumper.cc
            ${BP_ROOT}/client/linux/minidump_writer/minidump_writer.cc
            ${BP_ROOT}/client/linux/minidump_writer/pe_file.cc
            ${BP_ROOT}/client/minidump_file_writer.cc
            ${BP_ROOT}/common/convert_UTF.cc
            ${BP_ROOT}/common/md5.cc
            ${BP_ROOT}/common/string_conversion.cc
            ${BP_ROOT}/common/linux/elf_core_dump.cc
            ${BP_ROOT}/common/linux/elfutils.cc
            ${BP_ROOT}/common/linux/file_id.cc
            ${BP_ROOT}/common/linux/guid_creator.cc
            ${BP_ROOT}/common/linux/linux_libc_support.cc
            ${BP_ROOT}/common/linux/memory_mapped_file.cc
            ${BP_ROOT}/common/linux/safe_readlink.cc
        )
    elseif(WIN32)
    set(BP_CLIENT_SOURCES
        ${BP_ROOT}/client/windows/crash_generation/client_info.cc
        ${BP_ROOT}/client/windows/crash_generation/crash_generation_server.cc
        ${BP_ROOT}/client/windows/crash_generation/crash_generation_client.cc
        ${BP_ROOT}/client/windows/handler/exception_handler.cc
        ${BP_ROOT}/client/windows/crash_generation/minidump_generator.cc
        ${BP_ROOT}/common/windows/guid_string.cc
        ${BP_ROOT}/common/windows/string_utils.cc
    )

    else()
        message(FATAL_ERROR "Supported platforms: Windows and Linux")
    endif()

    add_library(${target} STATIC ${BP_CLIENT_SOURCES})

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_libraries(${target} PUBLIC Threads::Threads)
        target_compile_definitions(${target} PRIVATE HAVE_GETCONTEXT=1)
    endif()

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
