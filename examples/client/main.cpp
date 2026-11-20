#include <cstdio>
#include <string>
#include "client/windows/handler/exception_handler.h"

// Quote one Windows command-line argument, including trailing backslashes.
static std::wstring quote(const std::wstring& value)
{
    std::wstring result = L"\"";
    size_t slashes = 0;
    for (wchar_t c : value) {
        if (c == L'\\') { ++slashes; continue; }
        result.append(c == L'"' ? slashes * 2 + 1 : slashes, L'\\');
        slashes = 0;
        result += c;
    }
    result.append(slashes * 2, L'\\');
    return result + L"\"";
}

int wmain(int argc, wchar_t* argv[])
{
    if (argc != 3) {
        fwprintf(stderr, L"usage: %s <crash_handler executable> <dump directory>\n", argv[0]);
        return 1;
    }
    const std::wstring pipe = L"\\\\.\\pipe\\example_crash_" + std::to_wstring(GetCurrentProcessId());
    std::wstring command = quote(argv[1]) + L" --pipe " + quote(pipe)
        + L" --dumps " + quote(argv[2]);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(argv[1], command.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process)) {
        fwprintf(stderr, L"Cannot launch crash handler: %lu\n", GetLastError());
        return 1;
    }
    CloseHandle(process.hThread);
    const ULONGLONG deadline = GetTickCount64() + 10000;
    bool ready = false;
    while (GetTickCount64() < deadline) {
        if (WaitForSingleObject(process.hProcess, 0) != WAIT_TIMEOUT) break;
        if (WaitNamedPipeW(pipe.c_str(), 100)) { ready = true; break; }
        Sleep(10);
    }
    if (!ready) {
        fwprintf(stderr, L"Crash handler did not become ready\n");
        TerminateProcess(process.hProcess, 1);
        CloseHandle(process.hProcess);
        return 1;
    }
    google_breakpad::ExceptionHandler handler(
        L".", nullptr, nullptr, nullptr,
        google_breakpad::ExceptionHandler::HANDLER_ALL,
        MiniDumpNormal, pipe.c_str(), nullptr);
    if (!handler.IsOutOfProcess()) {
        fwprintf(stderr, L"Cannot register with crash handler\n");
        TerminateProcess(process.hProcess, 1);
        CloseHandle(process.hProcess);
        return 1;
    }
    CloseHandle(process.hProcess);
    volatile int* p = nullptr;
    *p = 42;
    return 0;
}
