// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

#include "core/color.h"

namespace lofibox::ui {

inline constexpr auto kBgRoot = core::rgba(5, 6, 8);
inline constexpr auto kBgPanel0 = core::rgba(10, 12, 15);
inline constexpr auto kBgPanel1 = core::rgba(16, 19, 24);
inline constexpr auto kBgPanel2 = core::rgba(23, 27, 33);
inline constexpr auto kChromeTopbar0 = core::rgba(43, 46, 51);
inline constexpr auto kChromeTopbar1 = core::rgba(16, 19, 23);
inline constexpr auto kDivider = core::rgba(42, 47, 54);
inline constexpr auto kTextPrimary = core::rgba(245, 247, 250);
inline constexpr auto kTextSecondary = core::rgba(199, 205, 211);
inline constexpr auto kTextMuted = core::rgba(141, 148, 156);
inline constexpr auto kFocusFill1 = core::rgba(47, 134, 255);
inline constexpr auto kFocusEdge = core::rgba(169, 219, 255);
inline constexpr auto kProgress = core::rgba(88, 168, 255);
inline constexpr auto kProgressStrong = core::rgba(47, 117, 255);
inline constexpr auto kGood = core::rgba(135, 217, 108);
inline constexpr auto kWarn = core::rgba(230, 179, 74);
inline constexpr auto kBad = core::rgba(214, 106, 95);

inline constexpr int kTopBarHeight = 20;
inline constexpr int kListTop = 28;
inline constexpr int kListRowHeight = 21;
inline constexpr int kMaxVisibleRows = 6;
inline constexpr int kListInset = 8;

inline constexpr std::string_view kUnknown = "UNKNOWN";

} // namespace lofibox::ui
