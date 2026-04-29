// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/widgets/tui_text.h"

#include "tui/tui_state.h"

namespace lofibox::tui::widgets {

std::string fitText(std::string_view text, int width)
{
    if (width <= 0) {
        return {};
    }
    auto value = truncateCells(std::string{text}, width);
    if (static_cast<int>(value.size()) < static_cast<int>(text.size()) && width > 1) {
        value = truncateCells(std::string{text}, width - 1) + "~";
    }
    return value;
}

std::string padRight(std::string text, int width)
{
    if (width <= 0) {
        return {};
    }
    text = fitText(text, width);
    while (static_cast<int>(text.size()) < width) {
        text.push_back(' ');
    }
    return text;
}

std::string joinNonEmpty(std::initializer_list<std::string_view> parts, std::string_view separator)
{
    std::string result{};
    for (const auto part : parts) {
        if (part.empty()) {
            continue;
        }
        if (!result.empty()) {
            result += separator;
        }
        result += part;
    }
    return result;
}

} // namespace lofibox::tui::widgets

