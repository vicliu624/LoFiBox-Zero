// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/tui_state.h"

#include <algorithm>
#include <sstream>

namespace lofibox::tui {

TuiFrame::TuiFrame(int frame_width, int frame_height)
    : width(std::max(0, frame_width)),
      height(std::max(0, frame_height)),
      cells(static_cast<std::size_t>(std::max(0, frame_width) * std::max(0, frame_height)))
{
}

bool TuiFrame::empty() const noexcept
{
    return width <= 0 || height <= 0 || cells.empty();
}

void TuiFrame::clear()
{
    for (auto& cell : cells) {
        cell = {};
    }
}

void TuiFrame::put(int row, int col, std::string glyph, TuiStyle style)
{
    if (row < 0 || col < 0 || row >= height || col >= width) {
        return;
    }
    cells[static_cast<std::size_t>(row * width + col)] = TuiCell{std::move(glyph), style};
}

void TuiFrame::write(int row, int col, std::string_view text, TuiStyle style)
{
    if (row < 0 || row >= height || col >= width) {
        return;
    }
    int current_col = std::max(0, col);
    for (std::size_t index = 0; index < text.size() && current_col < width;) {
        const unsigned char ch = static_cast<unsigned char>(text[index]);
        std::size_t count = 1;
        if ((ch & 0x80U) != 0U) {
            if ((ch & 0xE0U) == 0xC0U) count = 2;
            else if ((ch & 0xF0U) == 0xE0U) count = 3;
            else if ((ch & 0xF8U) == 0xF0U) count = 4;
        }
        if (index + count > text.size()) {
            break;
        }
        put(row, current_col, std::string{text.substr(index, count)}, style);
        index += count;
        ++current_col;
    }
}

void TuiFrame::writeLine(int row, std::string_view text, TuiStyle style)
{
    write(row, 0, text, style);
}

std::string TuiFrame::line(int row) const
{
    if (row < 0 || row >= height) {
        return {};
    }
    std::string result{};
    for (int col = 0; col < width; ++col) {
        result += cells[static_cast<std::size_t>(row * width + col)].glyph;
    }
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    return result;
}

std::string TuiFrame::toString() const
{
    std::ostringstream out{};
    for (int row = 0; row < height; ++row) {
        out << line(row);
        if (row + 1 < height) {
            out << '\n';
        }
    }
    return out.str();
}

TuiCharset tuiCharsetFromName(std::string_view name, TuiCharset fallback) noexcept
{
    if (name == "unicode") return TuiCharset::Unicode;
    if (name == "ascii") return TuiCharset::Ascii;
    if (name == "minimal") return TuiCharset::Minimal;
    return fallback;
}

TuiTheme tuiThemeFromName(std::string_view name, TuiTheme fallback) noexcept
{
    if (name == "dark") return TuiTheme::Dark;
    if (name == "light") return TuiTheme::Light;
    if (name == "amber") return TuiTheme::Amber;
    if (name == "mono") return TuiTheme::Mono;
    return fallback;
}

std::string layoutKindName(TuiLayoutKind kind)
{
    switch (kind) {
    case TuiLayoutKind::TooSmall: return "too-small";
    case TuiLayoutKind::Tiny: return "tiny";
    case TuiLayoutKind::Micro: return "micro";
    case TuiLayoutKind::Compact: return "compact";
    case TuiLayoutKind::Normal: return "normal";
    case TuiLayoutKind::Wide: return "wide";
    }
    return "normal";
}

std::string truncateCells(std::string text, int width)
{
    if (width <= 0) {
        return {};
    }
    int cells = 0;
    std::string result{};
    for (std::size_t index = 0; index < text.size() && cells < width;) {
        const unsigned char ch = static_cast<unsigned char>(text[index]);
        std::size_t count = 1;
        if ((ch & 0x80U) != 0U) {
            if ((ch & 0xE0U) == 0xC0U) count = 2;
            else if ((ch & 0xF0U) == 0xE0U) count = 3;
            else if ((ch & 0xF8U) == 0xF0U) count = 4;
        }
        if (index + count > text.size()) {
            break;
        }
        result.append(text.substr(index, count));
        index += count;
        ++cells;
    }
    return result;
}

} // namespace lofibox::tui

