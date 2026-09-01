// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_host_internal.h"

#include <cassert>
#include <filesystem>
#include <string>
#include <vector>

int main()
{
#if defined(__linux__)
    using lofibox::platform::host::runtime_detail::captureProcessOutput;
    using lofibox::platform::host::runtime_detail::runProcess;

    const std::filesystem::path shell{"/bin/sh"};
    const auto output = captureProcessOutput(shell, {"-c", "printf 'metadata output'"});
    assert(output);
    assert(*output == "metadata output");

    assert(runProcess(shell, {"-c", "exit 0"}));
    assert(!runProcess(shell, {"-c", "exit 7"}));
#endif
    return 0;
}
