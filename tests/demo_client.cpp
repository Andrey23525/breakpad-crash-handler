// demo_client.cpp
//
// Minimal Breakpad client used to exercise the crash handler for a screenshot
// and manual testing. It registers with the handler's named pipe, installs an
// unhandled-exception filter that requests an out-of-process dump, then
// dereferences a null pointer to trigger a real access violation.

#include <windows.h>

#include <cstdio>

#include "client/windows/crash_generation/crash_generation_client.h"

static google_breakpad::CrashGenerationClient* g_client = nullptr;

static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionPointers)
{
    if (g_client) {
        // Blocks until the handler writes the minidump.
        g_client->RequestDump(exceptionPointers);
    }
    TerminateProcess(GetCurrentProcess(), 0xC0000005u);
    return EXCEPTION_EXECUTE_HANDLER; // unreachable
}

int wmain(int argc, wchar_t* argv[])
{
    if (argc < 2) {
        fwprintf(stderr, L"usage: crash_demo_client.exe <pipe-name>\n");
        return 1;
    }

    // Suppress the Windows "has stopped working" dialog.
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

    google_breakpad::CrashGenerationClient client(argv[1], MiniDumpNormal, nullptr);
    if (!client.Register()) {
        fwprintf(stderr, L"failed to register with %s\n", argv[1]);
        return 1;
    }

    g_client = &client;
    SetUnhandledExceptionFilter(unhandledExceptionFilter);

    // Trigger a null-pointer dereference: access violation -> crash dump.
    volatile int* p = nullptr;
    *p = 42;

    return 0;
}
