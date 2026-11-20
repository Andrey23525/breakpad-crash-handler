#include "crash_handler/client.h"
#include <exception>
#include <cstdio>

int main(int argc, char** argv)
{
    if (argc != 3) return 2;
    try {
        // Project values are forwarded to the handler at startup; a normal
        // destruction must allow initialization again in the same process.
        crash_handler::Configuration configuration;
        configuration.productName = "Client lifecycle test";
        configuration.supportEmail = "test@example.invalid";
        configuration.uploadUrl = "https://example.invalid/api/upload";
        configuration.uploadFieldProduct = "product";
        configuration.uploadFieldReport = "report";
        configuration.uploadFieldDump = "dump";
        for (int i = 0; i != 2; ++i) {
            crash_handler::Client client(argv[1], argv[2], configuration);
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
