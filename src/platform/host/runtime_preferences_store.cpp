// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/host/runtime_preferences_store.h"
#include <charconv>
#include <fstream>
#include <string>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace lofibox::platform::host {
std::shared_ptr<app::RuntimePreferences> loadRuntimePreferences(const std::filesystem::path& path)
{
    std::uint32_t flags = app::RuntimePreferences::kDefaults;
    bool loaded{};
    std::ifstream input{path};
    std::string line;
    if (std::getline(input, line) && line.starts_with("lofibox-preferences-v1 ")) {
        const auto value = std::string_view{line}.substr(23);
        std::uint32_t parsed{};
        const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
        if (result.ec == std::errc{} && result.ptr == value.data() + value.size()
            && (parsed & ~app::RuntimePreferences::kValidMask) == 0) {
            flags = parsed;
            loaded = true;
        }
    }
    auto save = [path](std::uint32_t value) {
        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) return false;
        auto temporary = path;
        temporary += ".tmp";
        {
            std::ofstream output{temporary, std::ios::trunc};
            output << "lofibox-preferences-v1 " << value << '\n';
            output.flush();
            if (!output) { output.close(); std::filesystem::remove(temporary, error); return false; }
        }
#if defined(_WIN32)
        if (MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
#else
        std::filesystem::rename(temporary, path, error);
        if (!error) return true;
#endif
        std::filesystem::remove(temporary, error);
        return false;
    };
    return std::make_shared<app::RuntimePreferences>(flags, std::move(save), loaded);
}
}
