// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>

namespace lofibox::core {

inline constexpr int kDisplayWidth = 320;
inline constexpr int kDisplayHeight = 170;
inline constexpr int kListVisibleRows = 5;
inline constexpr int kScreenKeyBarTop = 142;

[[nodiscard]] constexpr int visibleListRows(int top, int row_height, int requested) noexcept
{
    const int available = std::max(1, (kScreenKeyBarTop - top) / std::max(1, row_height));
    return std::clamp(requested, 1, available);
}

} // namespace lofibox::core
