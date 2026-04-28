// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/hello_app.h"

#include <algorithm>
#include <cctype>
#include <string_view>

#include "core/bitmap_font.h"
#include "core/display_profile.h"

namespace lofibox::app {
namespace {

using core::rgba;

constexpr auto kBackground = rgba(9, 13, 20);
constexpr auto kPanel = rgba(18, 28, 41);
constexpr auto kHeader = rgba(11, 52, 85);
constexpr auto kBorder = rgba(52, 164, 224);
constexpr auto kTitle = rgba(188, 228, 250);
constexpr auto kText = rgba(245, 249, 252);
constexpr auto kMuted = rgba(120, 155, 180);
constexpr auto kAccent = rgba(255, 189, 46);
constexpr auto kInputFill = rgba(8, 12, 18);
constexpr auto kInputBorder = rgba(78, 94, 110);
constexpr auto kGood = rgba(155, 220, 116);

int centeredX(std::string_view text, int scale) noexcept
{
    return (core::kDisplayWidth - core::bitmap_font::measureText(text, scale)) / 2;
}

std::string clippedTail(const std::string& value, std::size_t max_length)
{
    if (value.size() <= max_length) {
        return value;
    }

    return value.substr(value.size() - max_length);
}

} // namespace

HelloApp::HelloApp()
    : started_at_(std::chrono::steady_clock::now())
{
}

void HelloApp::handleInput(const InputEvent& event) noexcept
{
    if (!event.label.empty()) {
        last_key_label_ = event.label;
    }

    switch (event.key) {
    case InputKey::Character:
        if (event.text != '\0') {
            if (typed_text_.size() >= 26) {
                typed_text_.erase(0, 1);
            }
            typed_text_.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(event.text))));
            status_text_ = "TEXT INPUT CAPTURED";
        }
        break;
    case InputKey::Backspace:
        if (!typed_text_.empty()) {
            typed_text_.pop_back();
        }
        status_text_ = "BACKSPACE LAST CHARACTER";
        break;
    case InputKey::Delete:
        status_text_ = "DELETE TRIGGERED";
        break;
    case InputKey::Enter:
        status_text_ = "OK / ENTER TRIGGERED";
        break;
    case InputKey::Tab:
        if (typed_text_.size() < 25) {
            typed_text_ += ' ';
        }
        status_text_ = "TAB INSERTED SPACE";
        break;
    case InputKey::F1:
        status_text_ = "F1 HELP TRIGGERED";
        break;
    case InputKey::F2:
        status_text_ = "F2 PLAY TRIGGERED";
        break;
    case InputKey::F3:
        status_text_ = "F3 PAUSE TRIGGERED";
        break;
    case InputKey::F4:
        status_text_ = "F4 PREVIOUS TRIGGERED";
        break;
    case InputKey::F5:
        status_text_ = "F5 NEXT TRIGGERED";
        break;
    case InputKey::F6:
        status_text_ = "F6 SHUFFLE TRIGGERED";
        break;
    case InputKey::F7:
        status_text_ = "F7 LOOP TRIGGERED";
        break;
    case InputKey::F8:
        status_text_ = "F8 REPEAT ONE TRIGGERED";
        break;
    case InputKey::F9:
        status_text_ = "F9 SEARCH TRIGGERED";
        break;
    case InputKey::F10:
        status_text_ = "F10 LIBRARY TRIGGERED";
        break;
    case InputKey::F11:
        status_text_ = "F11 QUEUE TRIGGERED";
        break;
    case InputKey::F12:
        status_text_ = "F12 SETTINGS TRIGGERED";
        break;
    case InputKey::Home:
        typed_text_.clear();
        status_text_ = "HOME CLEARED THE BUFFER";
        break;
    case InputKey::PageUp:
        status_text_ = "PAGE UP TRIGGERED";
        break;
    case InputKey::PageDown:
        status_text_ = "PAGE DOWN TRIGGERED";
        break;
    case InputKey::Insert:
        status_text_ = "INSERT TRIGGERED";
        break;
    case InputKey::Next:
        status_text_ = "NEXT BUTTON TRIGGERED";
        break;
    case InputKey::Power:
        status_text_ = "POWER SWITCH TOGGLED";
        break;
    case InputKey::Fn:
        status_text_ = "FN MODIFIER PRESSED";
        break;
    case InputKey::Ctrl:
        status_text_ = "CTRL MODIFIER PRESSED";
        break;
    case InputKey::Alt:
        status_text_ = "ALT MODIFIER PRESSED";
        break;
    case InputKey::Shift:
        status_text_ = "SHIFT MODIFIER PRESSED";
        break;
    case InputKey::Left:
        status_text_ = "LEFT NAVIGATION";
        break;
    case InputKey::Right:
        status_text_ = "RIGHT NAVIGATION";
        break;
    case InputKey::Up:
        status_text_ = "UP NAVIGATION";
        break;
    case InputKey::Down:
        status_text_ = "DOWN NAVIGATION";
        break;
    case InputKey::Unknown:
        status_text_ = "UNKNOWN INPUT";
        break;
    }
}

void HelloApp::render(core::Canvas& canvas) const noexcept
{
    canvas.clear(kBackground);
    canvas.fillRect(8, 8, core::kDisplayWidth - 16, core::kDisplayHeight - 16, kPanel);
    canvas.strokeRect(8, 8, core::kDisplayWidth - 16, core::kDisplayHeight - 16, kBorder, 2);
    canvas.fillRect(18, 18, core::kDisplayWidth - 36, 22, kHeader);

    core::bitmap_font::drawText(canvas, "LOFIBOX ZERO", centeredX("LOFIBOX ZERO", 2), 23, kTitle, 2);
    core::bitmap_font::drawText(canvas, "CARDPUTER FRONT SIM", centeredX("CARDPUTER FRONT SIM", 1), 52, kMuted, 1);

    canvas.fillRect(24, 66, core::kDisplayWidth - 48, 34, kInputFill);
    canvas.strokeRect(24, 66, core::kDisplayWidth - 48, 34, kInputBorder, 1);
    core::bitmap_font::drawText(canvas, "INPUT", 34, 72, kMuted, 1);
    core::bitmap_font::drawText(
        canvas,
        hasTypedText() ? visibleInput() : std::string("CLICK KEYS TO TYPE"),
        34,
        83,
        hasTypedText() ? kText : kMuted,
        1);

    canvas.fillRect(24, 108, core::kDisplayWidth - 48, 20, kInputFill);
    canvas.strokeRect(24, 108, core::kDisplayWidth - 48, 20, kInputBorder, 1);
    core::bitmap_font::drawText(canvas, "LAST", 34, 114, kMuted, 1);
    core::bitmap_font::drawText(canvas, visibleLastKey(), 86, 114, kGood, 1);

    canvas.fillRect(24, 134, core::kDisplayWidth - 48, 20, kInputFill);
    canvas.strokeRect(24, 134, core::kDisplayWidth - 48, 20, kInputBorder, 1);
    core::bitmap_font::drawText(canvas, visibleStatus(), 34, 140, kText, 1);

    if (blinkOn()) {
        canvas.fillRect(251, 87, 16, 4, kAccent);
    }
}

bool HelloApp::blinkOn() const noexcept
{
    const auto elapsed = std::chrono::steady_clock::now() - started_at_;
    const auto ticks = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() / 450;
    return (ticks % 2) == 0;
}

bool HelloApp::hasTypedText() const noexcept
{
    return !typed_text_.empty();
}

std::string HelloApp::visibleInput() const
{
    return clippedTail(typed_text_, 24);
}

std::string HelloApp::visibleStatus() const
{
    return clippedTail(status_text_, 28);
}

std::string HelloApp::visibleLastKey() const
{
    return clippedTail(last_key_label_, 10);
}

} // namespace lofibox::app
