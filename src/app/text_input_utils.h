// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace lofibox::app {

[[nodiscard]] bool isValidUtf8(std::string_view text) noexcept;
[[nodiscard]] std::string truncateUtf8(std::string_view text, std::size_t max_bytes);
bool appendUtf8Bounded(std::string& target, std::string_view committed_text, std::size_t max_bytes);
bool popLastUtf8Codepoint(std::string& target);

} // namespace lofibox::app
