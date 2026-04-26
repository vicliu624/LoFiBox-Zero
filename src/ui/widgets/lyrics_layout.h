// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include "ui/ui_models.h"

namespace lofibox::ui::widgets {

struct LyricDisplayLine {
    double timestamp_seconds{-1.0};
    std::string text{};
};

[[nodiscard]] std::vector<LyricDisplayLine> lyricDisplayLines(const LyricsContent& lyrics);
[[nodiscard]] int activeLyricIndex(const std::vector<LyricDisplayLine>& lines, double elapsed_seconds, int duration_seconds);

} // namespace lofibox::ui::widgets
