// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string_view>

#include "runtime/runtime_command.h"

namespace lofibox::tui {

[[nodiscard]] std::optional<runtime::RuntimeCommand> commandPaletteRuntimeCommand(std::string_view text);
[[nodiscard]] bool commandPaletteRejectsShell(std::string_view text) noexcept;

} // namespace lofibox::tui

