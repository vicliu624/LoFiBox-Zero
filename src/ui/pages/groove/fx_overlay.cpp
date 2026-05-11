// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/pages/groove/fx_overlay.h"

#include <array>

#include "ui/pages/groove/groove_ui_helpers.h"
#include "ui/ui_primitives.h"

namespace lofibox::ui::pages::groove {

void renderFxOverlay(core::Canvas& canvas, const FxOverlayView& view, const UiTheme& theme)
{
    drawGrooveFrame(canvas, theme, "PUNCH FX");
    constexpr std::array<const char*, 8> labels{"FILT", "STUT", "CRSH", "STOP", "DLY", "FRZ", "REV", "BRK"};
    for (int index = 0; index < 8; ++index) {
        const int row = index < 4 ? 0 : 1;
        const int col = index % 4;
        const int x = 22 + (col * 72);
        const int y = 48 + (row * 24);
        const bool armed = view.armedFx == static_cast<std::uint8_t>(index + 1);
        canvas.fillRect(x - 4, y - 4, 62, 17, armed ? theme.palette.warn : theme.palette.panel2);
        canvas.strokeRect(x - 4, y - 4, 62, 17, armed ? theme.palette.focus_edge : theme.palette.divider, 1);
        ::lofibox::ui::drawText(canvas, std::to_string(index + 1) + " " + labels[static_cast<std::size_t>(index)], x, y, armed ? theme.palette.background : theme.palette.text_primary, 1);
    }
    drawGrooveFooter(canvas, theme, "1-8 FX TOGGLE  FN+KEY RECORD");
}

} // namespace lofibox::ui::pages::groove
