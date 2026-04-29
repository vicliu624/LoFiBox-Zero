// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <initializer_list>
#include <string>
#include <string_view>

namespace lofibox::tui::widgets {

[[nodiscard]] std::string fitText(std::string_view text, int width);
[[nodiscard]] std::string padRight(std::string text, int width);
[[nodiscard]] std::string joinNonEmpty(std::initializer_list<std::string_view> parts, std::string_view separator);

} // namespace lofibox::tui::widgets
