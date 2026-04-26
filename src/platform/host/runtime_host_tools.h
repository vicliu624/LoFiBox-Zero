// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/runtime_services.h"
#include "platform/host/runtime_host_internal.h"

namespace lofibox::platform::host::runtime_detail {

class RuntimeTextParsing {
public:
    [[nodiscard]] static std::string trim(std::string value);
    [[nodiscard]] static std::string asciiLower(std::string value);
    [[nodiscard]] static std::string repairMetadataText(std::string value);
    [[nodiscard]] static std::vector<FilenameMetadataGuess> fallbackMetadataGuessesFromPath(const fs::path& path);
};

class RuntimeJsonTools {
public:
    [[nodiscard]] static std::optional<std::string> extractString(std::string_view json, std::string_view marker, std::size_t start = 0);
    [[nodiscard]] static std::optional<bool> parseBool(std::string_view json, std::string_view marker);
    [[nodiscard]] static std::optional<int> parseInt(std::string_view json, std::string_view marker);
    [[nodiscard]] static std::string escape(std::string_view value);
};

class RuntimeCachePaths {
public:
    [[nodiscard]] static std::string keyForPath(const fs::path& path);
    [[nodiscard]] static fs::path metadataPath(const SharedRuntimeCache& cache, std::string_view key);
    [[nodiscard]] static fs::path artworkPath(const SharedRuntimeCache& cache, std::string_view key);
    [[nodiscard]] static fs::path lyricsPath(const SharedRuntimeCache& cache, std::string_view key);
};

class HelperScriptResolver {
public:
    [[nodiscard]] static fs::path tagWriter();
    [[nodiscard]] static fs::path remoteMediaTool();
};

} // namespace lofibox::platform::host::runtime_detail
