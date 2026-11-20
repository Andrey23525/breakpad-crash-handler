#pragma once
#include <filesystem>
#include <memory>

namespace crash_handler {
// Create once, early in main, and keep alive while the application runs.
// Throws std::runtime_error on startup failure. Destruction stops the child.
// Paths use std::filesystem's native encoding; no Qt or Breakpad types escape.
class Client final {
public:
    Client(const std::filesystem::path& executable,
           const std::filesystem::path& dumpDirectory);
    ~Client();
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
