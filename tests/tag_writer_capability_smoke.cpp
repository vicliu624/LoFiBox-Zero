// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/host/runtime_provider_factories.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

int main()
{
    namespace fs = std::filesystem;
    const auto directory = fs::temp_directory_path() / ("lofibox-capability-" + std::to_string(::getpid()));
    fs::create_directories(directory);
    const auto script = directory / "python3";
    const auto calls = directory / "calls.txt";
    {
        std::ofstream output(script);
        output << "#!/bin/sh\nprintf 'call\\n' >> '" << calls.string() << "'\nexit 0\n";
    }
    fs::permissions(script, fs::perms::owner_all);
    const char* previous = std::getenv("PYTHON_EXECUTABLE");
    const std::string restore = previous ? previous : "";
    ::setenv("PYTHON_EXECUTABLE", script.c_str(), 1);
    const auto writer = lofibox::platform::host::createHostTagWriter();
    if (previous) ::setenv("PYTHON_EXECUTABLE", restore.c_str(), 1);
    else ::unsetenv("PYTHON_EXECUTABLE");
    for (int query = 0; query < 1000; ++query) {
        if (!writer->available()) {
            std::cerr << "Capability probe unexpectedly unavailable.\n";
            return 1;
        }
    }
    int count = 0;
    std::ifstream input(calls);
    std::string line;
    while (std::getline(input, line)) ++count;
    fs::remove_all(directory);
    if (count != 1) {
        std::cerr << "Expected one initialization probe for 1000 capability queries, observed " << count << '\n';
        return 1;
    }
    return 0;
}
