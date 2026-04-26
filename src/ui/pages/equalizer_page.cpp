// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/pages/equalizer_page.h"

#include <algorithm>
#include <array>

#include "ui/ui_primitives.h"
#include "ui/ui_theme.h"

namespace lofibox::ui::pages {
namespace {

inline constexpr auto kEqHot1 = core::rgba(255, 127, 42);
inline constexpr auto kEqSelected0 = core::rgba(127, 209, 255);
inline constexpr auto kEqSelected1 = core::rgba(47, 134, 255);

} // namespace

void renderEqualizerPage(core::Canvas& canvas, const EqualizerPageView& view)
{
    ::lofibox::ui::drawListPageFrame(canvas);
    ::lofibox::ui::drawTopBar(canvas, "EQUALIZER", true);
    canvas.fillRect(6, 24, 308, 140, ::lofibox::ui::kBgPanel1);
    canvas.strokeRect(6, 24, 308, 140, ::lofibox::ui::kDivider, 1);
    ::lofibox::ui::drawText(canvas, "+12", 54, 34, ::lofibox::ui::kTextMuted, 1);
    ::lofibox::ui::drawText(canvas, "0", 62, 66, ::lofibox::ui::kTextSecondary, 1);
    ::lofibox::ui::drawText(canvas, "-12", 50, 98, ::lofibox::ui::kTextMuted, 1);
    constexpr std::array<std::string_view, 10> labels{"31", "62", "125", "250", "500", "1K", "2K", "4K", "8K", "16K"};

    for (int index = 0; index < 10; ++index) {
        const int x = 82 + (index * 22);
        const int slider_y = 40;
        const int slider_h = 64;
        canvas.fillRect(x, slider_y, 18, slider_h, ::lofibox::ui::kBgPanel2);
        const int zero_y = slider_y + slider_h / 2;
        canvas.fillRect(x, zero_y, 18, 1, ::lofibox::ui::kDivider);
        const int gain = view.bands[static_cast<std::size_t>(index)];
        const int pixels = gain * 2;
        const auto fill = index == view.selected_band ? kEqSelected1 : kEqHot1;
        if (pixels >= 0) {
            canvas.fillRect(x + 2, zero_y - pixels, 14, pixels + 1, fill);
        } else {
            canvas.fillRect(x + 2, zero_y, 14, (-pixels) + 1, fill);
        }
        if (index == view.selected_band) {
            canvas.strokeRect(x, slider_y, 18, slider_h, kEqSelected0, 1);
        }
        ::lofibox::ui::drawText(canvas, labels[static_cast<std::size_t>(index)], x, 118, index == view.selected_band ? ::lofibox::ui::kTextPrimary : ::lofibox::ui::kTextMuted, 1);
    }
    ::lofibox::ui::drawText(canvas, view.preset_name, 120, 144, ::lofibox::ui::kTextSecondary, 1);
}

} // namespace lofibox::ui::pages
