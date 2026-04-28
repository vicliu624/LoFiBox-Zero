// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>
#include <string_view>
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
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    Home,
    PageUp,
    PageDown,
    Insert,
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
    std::string text{};

    InputEvent() = default;

    InputEvent(InputKey input_key, std::string input_label, std::string committed_text = {})
        : key(input_key)
        , label(std::move(input_label))
        , text(std::move(committed_text))
    {}

    InputEvent(InputKey input_key, std::string input_label, char committed_char)
        : key(input_key)
        , label(std::move(input_label))
        , text(committed_char == '\0' ? std::string{} : std::string(1, committed_char))
    {}
};

[[nodiscard]] inline InputEvent makeCharacterInput(char ch, std::string label = {})
{
    return InputEvent{InputKey::Character, std::move(label), ch};
}

[[nodiscard]] inline InputEvent makeCommittedTextInput(std::string text, std::string label = {})
{
    if (label.empty()) {
        label = text;
    }
    return InputEvent{InputKey::Character, std::move(label), std::move(text)};
}

[[nodiscard]] inline std::optional<char> singleAsciiText(const InputEvent& event) noexcept
{
    if (event.key != InputKey::Character || event.text.size() != 1U) {
        return std::nullopt;
    }
    const auto byte = static_cast<unsigned char>(event.text.front());
    if (byte > 0x7FU) {
        return std::nullopt;
    }
    return event.text.front();
}

} // namespace lofibox::app
