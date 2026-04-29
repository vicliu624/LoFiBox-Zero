// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "tui/tui_view_mode.h"

namespace lofibox::tui {

struct TerminalSize {
    int width{80};
    int height{24};
};

enum class TuiCharset {
    Unicode,
    Ascii,
    Minimal,
};

enum class TuiTheme {
    Dark,
    Light,
    Amber,
    Mono,
};

enum class TuiLayoutKind {
    TooSmall,
    Tiny,
    Micro,
    Compact,
    Normal,
    Wide,
};

enum class TuiInputMode {
    Normal,
    Search,
    Command,
    Text,
    Modal,
};

enum class TuiKey {
    Unknown,
    Character,
    Space,
    Enter,
    Escape,
    Tab,
    BackTab,
    ArrowLeft,
    ArrowRight,
    ArrowUp,
    ArrowDown,
    ShiftArrowLeft,
    ShiftArrowRight,
    CtrlD,
    CtrlU,
    Backspace,
    Paste,
};

struct TuiKeyEvent {
    TuiKey key{TuiKey::Unknown};
    char32_t codepoint{0};
    bool ctrl{false};
    bool alt{false};
    bool shift{false};
    std::string text{};
};

struct TuiOptions {
    TuiViewMode initial_view{TuiViewMode::Dashboard};
    TuiCharset charset{TuiCharset::Unicode};
    TuiTheme theme{TuiTheme::Dark};
    bool color{true};
    bool once{false};
    std::chrono::milliseconds refresh{100};
    std::optional<std::string> runtime_socket{};
    std::optional<TuiLayoutKind> layout_override{};
};

struct TuiStyle {
    bool bold{false};
    bool inverse{false};
    int color{0};
};

struct TuiCell {
    std::string glyph{" "};
    TuiStyle style{};
};

struct TuiFrame {
    int width{0};
    int height{0};
    std::vector<TuiCell> cells{};

    TuiFrame() = default;
    TuiFrame(int frame_width, int frame_height);

    [[nodiscard]] bool empty() const noexcept;
    void clear();
    void put(int row, int col, std::string glyph, TuiStyle style = {});
    void write(int row, int col, std::string_view text, TuiStyle style = {});
    void writeLine(int row, std::string_view text, TuiStyle style = {});
    [[nodiscard]] std::string line(int row) const;
    [[nodiscard]] std::string toString() const;
};

[[nodiscard]] TuiCharset tuiCharsetFromName(std::string_view name, TuiCharset fallback = TuiCharset::Unicode) noexcept;
[[nodiscard]] TuiTheme tuiThemeFromName(std::string_view name, TuiTheme fallback = TuiTheme::Dark) noexcept;
[[nodiscard]] std::string layoutKindName(TuiLayoutKind kind);
[[nodiscard]] std::string truncateCells(std::string text, int width);

} // namespace lofibox::tui
