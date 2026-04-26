// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <utility>

namespace lofibox::app {

enum class InputKey {
    Unknown = 0,
    Character,
    Backspace,
    Delete,
    Enter,
    Tab,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    Home,
    Next,
    Power,
    Fn,
    Ctrl,
    Alt,
    Shift,
    Left,
    Right,
    Up,
    Down,
};

struct InputEvent {
    InputKey key{InputKey::Unknown};
    std::string label{};
    char text{'\0'};
};

[[nodiscard]] inline InputEvent makeCharacterInput(char ch, std::string label = {})
{
    return InputEvent{InputKey::Character, std::move(label), ch};
}

} // namespace lofibox::app
