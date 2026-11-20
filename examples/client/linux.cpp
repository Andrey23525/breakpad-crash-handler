#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <string>
#include <unistd.h>
#include <sys/prctl.h>
#include "client/linux/crash_generation/crash_generation_server.h"
#include "client/linux/handler/exception_handler.h"

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s <crash_handler executable> <dump directory>\n", argv[0]);
        return 1;
    }
    int server, client;
    if (!google_breakpad::CrashGenerationServer::CreateReportChannel(&server, &client)) return 1;
    const pid_t child = fork();
    if (child < 0) { close(server); close(client); return 1; }
    if (child == 0) {
        close(client);
        const std::string fd = std::to_string(server);
        execl(argv[1], argv[1], "--server-fd", fd.c_str(), "--dumps", argv[2], nullptr);
        perror("exec crash_handler");
        _exit(1);
    }
    close(server);
    if (prctl(PR_SET_PTRACER, child, 0, 0, 0) != 0) {
        perror("PR_SET_PTRACER");
        close(client);
        kill(child, SIGTERM);
        return 1;
    }
    google_breakpad::MinidumpDescriptor descriptor(".");
    google_breakpad::ExceptionHandler handler(descriptor, nullptr, nullptr, nullptr, true, client);
    raise(SIGSEGV);
    return 1;
}
