#include "crash_handler/client.h"
#include <exception>
#include <cstdio>

int main(int argc, char** argv)
{
    if (argc != 3) return 2;
    try {
        // Normal destruction must allow initialization again in the same process.
        for (int i = 0; i != 2; ++i) {
            crash_handler::Client client(argv[1], argv[2]);
        }
        try {
            crash_handler::Client invalid(
                std::filesystem::path(argv[2]) / "missing-handler", argv[2]);
            return 3;
        } catch (const std::exception&) {}
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
    return 0;
}
