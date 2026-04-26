// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/input_event.h"

namespace lofibox::app {

enum class UserAction {
    None,
    Up,
    Down,
    Left,
    Right,
    Confirm,
    Back,
    Home,
    NextTrack,
};

[[nodiscard]] UserAction mapInput(const InputEvent& event);

} // namespace lofibox::app
