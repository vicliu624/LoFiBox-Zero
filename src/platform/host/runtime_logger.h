// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

namespace lofibox::platform::host {

enum class RuntimeLogLevel {
    Debug,
    Info,
    Warn,
    Error,
};

void logRuntime(RuntimeLogLevel level, std::string_view category, std::string_view message);

} // namespace lofibox::platform::host
