#pragma once
#include <filesystem>
#include <memory>
#include <string>

namespace crash_handler {
// Project-specific values forwarded to the handler process at startup, so the
// protected application owns them instead of them being compiled into the
// prebuilt handler. Fields left empty keep the handler's built-in defaults;
// all strings are UTF-8.
struct Configuration final {
    std::string productName;
    std::string supportEmail;
    std::string uploadUrl;
    std::string uploadFieldProduct;
    std::string uploadFieldReport;
    std::string uploadFieldDump;
};

// Create once, early in main, and keep alive while the application runs.
// Throws std::runtime_error on startup failure. Destruction stops the child.
// Paths use std::filesystem's native encoding; no Qt or Breakpad types escape.
class Client final {
public:
    Client(const std::filesystem::path& executable,
           const std::filesystem::path& dumpDirectory,
           const Configuration& configuration = Configuration());
    ~Client();
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
