// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <filesystem>
#include <memory>
#include "app/runtime_preferences.h"

namespace lofibox::platform::host {
[[nodiscard]] std::shared_ptr<app::RuntimePreferences> loadRuntimePreferences(const std::filesystem::path& path);
}
