#include <cstdio>
#include <exception>
#include "crash_handler/client.h"

int main(int argc, char** argv)
{
    if (argc != 3) {
        std::fprintf(stderr, "usage: %s <crash_handler executable> <dump directory>\n", argv[0]);
        return 1;
    }
    try {
        crash_handler::Client crashHandler(argv[1], argv[2]);
        // Keep crashHandler alive throughout your application.
        volatile int* p = nullptr;
        *p = 42;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
