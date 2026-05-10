// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

#include "core/canvas.h"
#include "ui/ui_theme.h"

namespace lofibox::ui::pages::groove {

void drawGrooveFrame(core::Canvas& canvas, const UiTheme& theme, std::string_view title);
void drawGrooveFooter(core::Canvas& canvas, const UiTheme& theme, std::string_view text);
void drawGrooveValueRow(core::Canvas& canvas, const UiTheme& theme, int y, std::string_view label, std::string_view value, bool selected = false);

} // namespace lofibox::ui::pages::groove
