// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>

#include "core/canvas.h"

namespace lofibox::platform::host {

[[nodiscard]] std::optional<core::Canvas> loadPngCanvas(const std::filesystem::path& path);

} // namespace lofibox::platform::host
