// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>

namespace lofibox::platform::host::runtime_paths {
namespace fs = std::filesystem;

[[nodiscard]] fs::path configHome();
[[nodiscard]] fs::path dataHome();
[[nodiscard]] fs::path cacheHome();
[[nodiscard]] fs::path stateHome();
[[nodiscard]] fs::path runtimeHome();
[[nodiscard]] fs::path temporaryHome();

[[nodiscard]] fs::path appConfigDir();
[[nodiscard]] fs::path appDataDir();
[[nodiscard]] fs::path appCacheDir();
[[nodiscard]] fs::path appStateDir();
[[nodiscard]] fs::path appRuntimeDir();
[[nodiscard]] fs::path appTemporaryDir();
[[nodiscard]] fs::path runtimeLogPath();

} // namespace lofibox::platform::host::runtime_paths
