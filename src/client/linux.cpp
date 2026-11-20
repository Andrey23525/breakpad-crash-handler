#include "crash_handler/client.h"
#include <cerrno>
#include <csignal>
#include <stdexcept>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include "client/linux/crash_generation/crash_generation_server.h"
#include "client/linux/handler/exception_handler.h"

namespace crash_handler {
struct Client::Impl {
    pid_t child = -1;
    int socket = -1;
    std::unique_ptr<google_breakpad::ExceptionHandler> handler;
    ~Impl() {
        handler.reset();
        if (socket >= 0) close(socket);
        if (child > 0) {
            kill(child, SIGKILL);
            while (waitpid(child, nullptr, 0) < 0 && errno == EINTR) {}
        }
    }
};
Client::Client(const std::filesystem::path& executable,
               const std::filesystem::path& dumpDirectory)
    : impl_(std::make_unique<Impl>())
{
    std::filesystem::create_directories(dumpDirectory);
    int server, client;
    if (!google_breakpad::CrashGenerationServer::CreateReportChannel(&server, &client))
        throw std::runtime_error("Cannot create report channel");
    impl_->socket = client;
    int status[2];
    if (pipe2(status, O_CLOEXEC) != 0) {
        close(server);
        throw std::runtime_error("Cannot create startup channel");
    }
    // Allocate before fork; the child only invokes async-signal-safe functions.
    const std::string fd = std::to_string(server);
    impl_->child = fork();
    if (impl_->child == 0) {
        close(client);
        close(status[0]);
        // Breakpad creates the server endpoint with FD_CLOEXEC. Preserve only
        // this endpoint across exec so the handler receives the report channel.
        if (fcntl(server, F_SETFD, 0) == 0)
            execl(executable.c_str(), executable.c_str(), "--server-fd", fd.c_str(),
                  "--dumps", dumpDirectory.c_str(), static_cast<char*>(nullptr));
        const int error = errno;
        const auto ignored = write(status[1], &error, sizeof(error));
        (void)ignored;
        _exit(1);
    }
    close(server);
    close(status[1]);
    if (impl_->child < 0) {
        close(status[0]);
        throw std::runtime_error("Cannot fork crash handler");
    }
    int error = 0;
    ssize_t count;
    do { count = read(status[0], &error, sizeof(error)); } while (count < 0 && errno == EINTR);
    close(status[0]);
    if (count != 0) throw std::runtime_error("Cannot execute crash handler");
    if (prctl(PR_SET_PTRACER, impl_->child, 0, 0, 0) != 0)
        throw std::runtime_error("Cannot grant crash handler ptrace permission");
    google_breakpad::MinidumpDescriptor descriptor(".");
    impl_->handler = std::make_unique<google_breakpad::ExceptionHandler>(
        descriptor, nullptr, nullptr, nullptr, true, client);
}
Client::~Client() = default;
}
