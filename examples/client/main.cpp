// main.cpp
//
// Minimal example of a client that reports crashes to the out-of-process
// Breakpad crash handler over a named pipe. Build with the root CMake project
// (target `example_client`), run the handler first, then crash this client.

#include <cwchar>
#include <string>

#include "client/windows/handler/exception_handler.h"

int wmain(int argc, wchar_t* argv[])
{
    if (argc < 2) {
        wprintf(L"usage: example_client.exe <pipe-name>\n");
        return 1;
    }

    const std::wstring pipeName = argv[1];

    // Register with the crash handler. Supplying a pipe name selects
    // out-of-process dump generation: the handler (server) writes the minidump.
    google_breakpad::ExceptionHandler handler(
        L".",                                            // dump path (unused out-of-process)
        nullptr,                                         // filter callback
        nullptr,                                         // minidump callback
        nullptr,                                         // callback context
        google_breakpad::ExceptionHandler::HANDLER_ALL,  // handler types
        MiniDumpNormal,                                  // dump type
        pipeName.c_str(),                                // named pipe -> out-of-process
        nullptr);                                        // custom client info

    // Application code goes here. Dereference a null pointer to demonstrate a
    // crash: the handler shows its window and writes the dump to its --dumps
    // directory.
    volatile int* p = nullptr;
    *p = 42;

    return 0;
}
