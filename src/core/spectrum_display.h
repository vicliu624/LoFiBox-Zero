// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cmath>

#include "core/display_profile.h"

namespace lofibox::core {

inline constexpr int kSpectrumTop = 24;
inline constexpr int kSpectrumBaseline = kScreenKeyBarTop - 5;
inline constexpr int kSpectrumMaxHeight = kSpectrumBaseline - kSpectrumTop + 1;

// Shared by Now Playing and the Cardputer indicator adapter, so the rightmost
// column and its lights use the same finite, clamped, quantized display value.
[[nodiscard]] inline int spectrumBarHeight(float energy) noexcept
{
    if (!std::isfinite(energy) || energy <= 0.0F) return 0;
    const float visible = std::sqrt(std::sqrt(std::clamp(energy * 4.0F, 0.0F, 1.0F)));
    return static_cast<int>(std::round(visible * static_cast<float>(kSpectrumMaxHeight)));
}

} // namespace lofibox::core
