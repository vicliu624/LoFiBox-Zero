// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/tui_model.h"

#include <utility>

namespace lofibox::tui {

TuiModel makeTuiModel(TuiOptions options)
{
    TuiModel model{};
    model.view = options.initial_view;
    model.runtime_socket = options.runtime_socket.value_or(std::string{});
    model.options = std::move(options);
    return model;
}

void updateTuiSnapshot(TuiModel& model, runtime::RuntimeSnapshot snapshot)
{
    model.snapshot = std::move(snapshot);
    model.runtime_connected = true;
    model.runtime_error.clear();
}

void markTuiDisconnected(TuiModel& model, std::string message)
{
    model.runtime_connected = false;
    model.runtime_error = std::move(message);
}

} // namespace lofibox::tui

