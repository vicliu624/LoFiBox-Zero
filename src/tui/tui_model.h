// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "runtime/runtime_snapshot.h"
#include "tui/tui_state.h"

namespace lofibox::tui {

struct TuiModel {
    TuiOptions options{};
    TuiViewMode view{TuiViewMode::Dashboard};
    TuiInputMode input_mode{TuiInputMode::Normal};
    runtime::RuntimeSnapshot snapshot{};
    bool runtime_connected{false};
    std::string runtime_error{};
    std::string runtime_socket{};
    int focused_row{0};
    int scroll{0};
    std::string search_buffer{};
    std::string command_buffer{};
    bool help_open{false};
    bool quit_requested{false};
};

[[nodiscard]] TuiModel makeTuiModel(TuiOptions options);
void updateTuiSnapshot(TuiModel& model, runtime::RuntimeSnapshot snapshot);
void markTuiDisconnected(TuiModel& model, std::string message);

} // namespace lofibox::tui

