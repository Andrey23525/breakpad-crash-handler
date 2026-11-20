#include "crash_handler/client.h"
#include <stdexcept>
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

namespace crash_handler {
struct Client::Impl {
    HANDLE process = nullptr;
    std::unique_ptr<google_breakpad::ExceptionHandler> handler;
    ~Impl() {
        handler.reset();
        if (process) {
            if (WaitForSingleObject(process, 0) == WAIT_TIMEOUT)
                TerminateProcess(process, 0);
            WaitForSingleObject(process, 5000);
            CloseHandle(process);
        }
    }
};
Client::Client(const std::filesystem::path& executable,
               const std::filesystem::path& dumpDirectory)
    : impl_(std::make_unique<Impl>())
{
    std::filesystem::create_directories(dumpDirectory);
    const std::wstring pipe = L"\\\\.\\pipe\\crash_client_" +
        std::to_wstring(GetCurrentProcessId());
    std::wstring command = quote(executable.wstring()) + L" --pipe " + quote(pipe)
        + L" --dumps " + quote(dumpDirectory.wstring());
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process))
        throw std::runtime_error("Cannot launch crash handler: " + std::to_string(GetLastError()));
    impl_->process = process.hProcess;
    CloseHandle(process.hThread);
    const ULONGLONG deadline = GetTickCount64() + 10000;
    bool ready = false;
    while (GetTickCount64() < deadline) {
        if (WaitForSingleObject(process.hProcess, 0) != WAIT_TIMEOUT) break;
        if (WaitNamedPipeW(pipe.c_str(), 100)) { ready = true; break; }
        Sleep(10);
    }
    if (!ready) throw std::runtime_error("Crash handler did not become ready");
    impl_->handler = std::make_unique<google_breakpad::ExceptionHandler>(
        L".", nullptr, nullptr, nullptr,
        google_breakpad::ExceptionHandler::HANDLER_ALL,
        MiniDumpNormal, pipe.c_str(), nullptr);
    if (!impl_->handler->IsOutOfProcess())
        throw std::runtime_error("Cannot register with crash handler");
}
Client::~Client() = default;
}
