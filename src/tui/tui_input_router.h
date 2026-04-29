// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>

#include "runtime/runtime_command.h"
#include "tui/tui_model.h"

namespace lofibox::tui {

enum class TuiInputActionKind {
    None,
    Quit,
    ChangeView,
    RuntimeCommand,
    OpenSearch,
    OpenCommandPalette,
    ToggleHelp,
};

struct TuiInputAction {
    TuiInputActionKind kind{TuiInputActionKind::None};
    TuiViewMode view{TuiViewMode::Dashboard};
    std::optional<runtime::RuntimeCommand> command{};
};

[[nodiscard]] TuiInputAction routeTuiInput(TuiModel& model, const TuiKeyEvent& event);

} // namespace lofibox::tui

