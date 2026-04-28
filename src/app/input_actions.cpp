// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/input_actions.h"

#include <cctype>

namespace lofibox::app {
namespace {

UserAction mapCharacterAction(char ch)
{
    switch (static_cast<char>(std::toupper(static_cast<unsigned char>(ch)))) {
    case 'W':
    case 'K':
        return UserAction::Up;
    case 'S':
    case 'J':
    case 'X':
        return UserAction::Down;
    case 'A':
    case 'H':
    case 'Q':
        return UserAction::Back;
    case 'D':
    case 'L':
        return UserAction::Confirm;
    case ',':
        return UserAction::Left;
    case '.':
    case 'C':
        return UserAction::Right;
    default:
        return UserAction::None;
    }
}

} // namespace

UserAction mapInput(const InputEvent& event)
{
    switch (event.key) {
    case InputKey::Up:
        return UserAction::Up;
    case InputKey::Down:
        return UserAction::Down;
    case InputKey::Left:
        return UserAction::Left;
    case InputKey::Right:
        return UserAction::Right;
    case InputKey::Enter:
        return UserAction::Confirm;
    case InputKey::Backspace:
        return UserAction::Back;
    case InputKey::Delete:
    case InputKey::F1:
    case InputKey::F2:
    case InputKey::F3:
    case InputKey::F4:
    case InputKey::F5:
    case InputKey::F6:
    case InputKey::F7:
    case InputKey::F8:
    case InputKey::F9:
    case InputKey::F10:
    case InputKey::F11:
    case InputKey::F12:
    case InputKey::Insert:
        return UserAction::None;
    case InputKey::Home:
        return UserAction::Home;
    case InputKey::PageUp:
        return UserAction::PageUp;
    case InputKey::PageDown:
        return UserAction::PageDown;
    case InputKey::Next:
        return UserAction::NextTrack;
    case InputKey::Character:
        return mapCharacterAction(event.text);
    default:
        return UserAction::None;
    }
}

} // namespace lofibox::app
