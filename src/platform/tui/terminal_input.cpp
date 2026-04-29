// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/tui/terminal_input.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#if defined(__unix__) || defined(__APPLE__)
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace lofibox::platform::tui {
namespace {

std::optional<char32_t> decodeUtf8Codepoint(std::string_view input)
{
    if (input.empty()) {
        return std::nullopt;
    }
    const auto byte0 = static_cast<unsigned char>(input[0]);
    if (byte0 < 0x80U) {
        return static_cast<char32_t>(byte0);
    }
    std::size_t count = 0;
    char32_t value = 0;
    if ((byte0 & 0xE0U) == 0xC0U) {
        count = 2;
        value = static_cast<char32_t>(byte0 & 0x1FU);
    } else if ((byte0 & 0xF0U) == 0xE0U) {
        count = 3;
        value = static_cast<char32_t>(byte0 & 0x0FU);
    } else if ((byte0 & 0xF8U) == 0xF0U) {
        count = 4;
        value = static_cast<char32_t>(byte0 & 0x07U);
    } else {
        return std::nullopt;
    }
    if (input.size() < count) {
        return std::nullopt;
    }
    for (std::size_t index = 1; index < count; ++index) {
        const auto byte = static_cast<unsigned char>(input[index]);
        if ((byte & 0xC0U) != 0x80U) {
            return std::nullopt;
        }
        value = static_cast<char32_t>((value << 6U) | (byte & 0x3FU));
    }
    return value;
}

bool isTextControl(char ch) noexcept
{
    const auto value = static_cast<unsigned char>(ch);
    return value < 0x20U && ch != '\t' && ch != '\n' && ch != '\r';
}

std::string stripAnsiSequences(std::string_view input)
{
    std::string output{};
    output.reserve(input.size());
    for (std::size_t index = 0; index < input.size();) {
        if (input[index] == '\x1b') {
            ++index;
            if (index < input.size() && input[index] == '[') {
                ++index;
                while (index < input.size()) {
                    const unsigned char ch = static_cast<unsigned char>(input[index++]);
                    if (ch >= 0x40U && ch <= 0x7EU) {
                        break;
                    }
                }
                continue;
            }
            continue;
        }
        output.push_back(input[index++]);
    }
    return output;
}

} // namespace

#if defined(__unix__) || defined(__APPLE__)
struct TerminalInput::TermiosHolder {
    termios original{};
};
#endif

std::string sanitizeTerminalPaste(std::string_view input)
{
    auto stripped = stripAnsiSequences(input);
    std::string output{};
    output.reserve(std::min<std::size_t>(stripped.size(), 4096));
    for (const char ch : stripped) {
        if (output.size() >= 4096) {
            break;
        }
        if (isTextControl(ch)) {
            continue;
        }
        if (ch == '\r' || ch == '\n') {
            output.push_back(' ');
            continue;
        }
        output.push_back(ch);
    }
    return output;
}

std::optional<lofibox::tui::TuiKeyEvent> decodeTerminalInputBytes(std::string_view input)
{
    if (input.empty()) {
        return std::nullopt;
    }
    constexpr std::string_view paste_begin{"\x1b[200~"};
    constexpr std::string_view paste_end{"\x1b[201~"};
    if (input.starts_with(paste_begin)) {
        auto text = input.substr(paste_begin.size());
        if (const auto end = text.find(paste_end); end != std::string_view::npos) {
            text = text.substr(0, end);
        }
        const auto sanitized = sanitizeTerminalPaste(text);
        return lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::Paste, 0, false, false, false, sanitized};
    }
    if (input == " ") return lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::Space};
    if (input == "\n" || input == "\r") return lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::Enter};
    if (input == "\t") return lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::Tab};
    if (input == "\x1b[Z") return lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::BackTab};
    if (input == "\x04") return lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::CtrlD, 0, true};
    if (input == "\x15") return lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::CtrlU, 0, true};
    if (input == "\x7f" || input == "\b") return lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::Backspace};
    if (input == "\x1b") return lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::Escape};
    if (input == "\x1b[D") return lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::ArrowLeft};
    if (input == "\x1b[C") return lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::ArrowRight};
    if (input == "\x1b[A") return lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::ArrowUp};
    if (input == "\x1b[B") return lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::ArrowDown};
    if (input == "\x1b[1;2D" || input == "\x1b[2D") return lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::ShiftArrowLeft};
    if (input == "\x1b[1;2C" || input == "\x1b[2C") return lofibox::tui::TuiKeyEvent{lofibox::tui::TuiKey::ShiftArrowRight};
    if (input.size() == 1 && static_cast<unsigned char>(input.front()) >= 32U) {
        return lofibox::tui::TuiKeyEvent{
            lofibox::tui::TuiKey::Character,
            static_cast<unsigned char>(input.front()),
            false,
            false,
            false,
            std::string{input}};
    }
    if (const auto codepoint = decodeUtf8Codepoint(input)) {
        return lofibox::tui::TuiKeyEvent{
            lofibox::tui::TuiKey::Character,
            *codepoint,
            false,
            false,
            false,
            std::string{input}};
    }
    return std::nullopt;
}

TerminalInput::~TerminalInput()
{
    leaveRawMode();
#if defined(__unix__) || defined(__APPLE__)
    delete termios_;
#endif
}

void TerminalInput::enterRawMode()
{
#if defined(__unix__) || defined(__APPLE__)
    if (raw_) {
        return;
    }
    if (termios_ == nullptr) {
        termios_ = new TermiosHolder{};
    }
    if (::tcgetattr(STDIN_FILENO, &termios_->original) != 0) {
        return;
    }
    auto raw = termios_->original;
    raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (::tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) {
        raw_ = true;
        saved_ = true;
    }
#endif
}

void TerminalInput::leaveRawMode()
{
#if defined(__unix__) || defined(__APPLE__)
    if (raw_ && saved_ && termios_ != nullptr) {
        (void)::tcsetattr(STDIN_FILENO, TCSANOW, &termios_->original);
    }
#endif
    raw_ = false;
}

std::optional<lofibox::tui::TuiKeyEvent> TerminalInput::poll(std::chrono::milliseconds timeout)
{
#if defined(__unix__) || defined(__APPLE__)
    pollfd fd{};
    fd.fd = STDIN_FILENO;
    fd.events = POLLIN;
    const int ready = ::poll(&fd, 1, static_cast<int>(timeout.count()));
    if (ready <= 0 || (fd.revents & POLLIN) == 0) {
        return std::nullopt;
    }
    std::array<char, 4096> bytes{};
    const auto count = ::read(STDIN_FILENO, bytes.data(), bytes.size());
    if (count <= 0) {
        return std::nullopt;
    }
    return decodeTerminalInputBytes(std::string_view{bytes.data(), static_cast<std::size_t>(count)});
#else
    (void)timeout;
#endif
    return std::nullopt;
}

} // namespace lofibox::platform::tui
