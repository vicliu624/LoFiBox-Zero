// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_paths.h"

#include <filesystem>
#include <iostream>
#include <string>

#if defined(_WIN32)
int main()
{
    std::cout << "XDG path smoke is Linux-specific; skipping on this platform.\n";
    return 0;
}
#else
namespace fs = std::filesystem;
namespace runtime_paths = lofibox::platform::host::runtime_paths;

namespace {

bool expectPath(const fs::path& actual, const fs::path& expected, const char* label)
{
    if (actual != expected) {
        std::cerr << label << " mismatch.\nExpected: " << expected << "\nActual: " << actual << '\n';
        return false;
    }
    return true;
}

} // namespace

int main()
{
    if (!expectPath(runtime_paths::appConfigDir(), fs::path{"/tmp/lofibox-xdg-config"} / "lofibox", "config")) {
        return 1;
    }
    if (!expectPath(runtime_paths::appDataDir(), fs::path{"/tmp/lofibox-xdg-data"} / "lofibox", "data")) {
        return 1;
    }
    if (!expectPath(runtime_paths::appCacheDir(), fs::path{"/tmp/lofibox-xdg-cache"} / "lofibox", "cache")) {
        return 1;
    }
    if (!expectPath(runtime_paths::appStateDir(), fs::path{"/tmp/lofibox-xdg-state"} / "lofibox", "state")) {
        return 1;
    }
    if (!expectPath(runtime_paths::appRuntimeDir(), fs::path{"/tmp/lofibox-xdg-runtime"} / "lofibox", "runtime")) {
        return 1;
    }
    if (!expectPath(runtime_paths::runtimeLogPath(), fs::path{"/tmp/lofibox-xdg-state"} / "lofibox" / "runtime.log", "runtime log")) {
        return 1;
    }
    return 0;
}
#endif
