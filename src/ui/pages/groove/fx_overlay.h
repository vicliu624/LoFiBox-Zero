// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

#include "core/canvas.h"
#include "ui/ui_theme.h"

namespace lofibox::ui::pages::groove {

struct FxOverlayView {
    std::uint8_t heldFx{0};
};

void renderFxOverlay(core::Canvas& canvas, const FxOverlayView& view, const UiTheme& theme);

} // namespace lofibox::ui::pages::groove
