// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include "tui/tui_input_router.h"

int main()
{
    auto model = lofibox::tui::makeTuiModel({});
    auto action = lofibox::tui::routeTuiInput(model, lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::Space});
    if (action.kind != lofibox::tui::TuiInputActionKind::RuntimeCommand
        || !action.command
        || action.command->kind != lofibox::runtime::RuntimeCommandKind::PlaybackToggle) {
        std::cerr << "Expected Space to produce PlaybackToggle.\n";
        return 1;
    }

    action = lofibox::tui::routeTuiInput(model, lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::Character, U'n'});
    const auto* step = action.command ? action.command->payload.get<lofibox::runtime::QueueStepPayload>() : nullptr;
    if (action.kind != lofibox::tui::TuiInputActionKind::RuntimeCommand || step == nullptr || step->delta != 1) {
        std::cerr << "Expected n to produce QueueStep(+1).\n";
        return 1;
    }

    action = lofibox::tui::routeTuiInput(model, lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::Character, U'q'});
    if (action.kind != lofibox::tui::TuiInputActionKind::Quit || action.command) {
        std::cerr << "Expected q to quit TUI without sending stop.\n";
        return 1;
    }

    model = lofibox::tui::makeTuiModel({});
    action = lofibox::tui::routeTuiInput(model, lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::Character, U':'});
    if (model.input_mode != lofibox::tui::TuiInputMode::Command) {
        std::cerr << "Expected : to open command mode.\n";
        return 1;
    }
    action = lofibox::tui::routeTuiInput(model, lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::Paste, 0, false, false, false, "next"});
    if (action.command || model.command_buffer != "next") {
        std::cerr << "Expected paste to fill command buffer without executing.\n";
        return 1;
    }
    action = lofibox::tui::routeTuiInput(model, lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::Enter});
    step = action.command ? action.command->payload.get<lofibox::runtime::QueueStepPayload>() : nullptr;
    if (action.kind != lofibox::tui::TuiInputActionKind::RuntimeCommand || step == nullptr || step->delta != 1) {
        std::cerr << "Expected Enter in command mode to submit command palette runtime command.\n";
        return 1;
    }
    return 0;
}
