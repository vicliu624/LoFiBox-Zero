// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/pages/about_page.h"

#include "app/ui/ui_primitives.h"
#include "app/ui/ui_theme.h"

namespace lofibox::app::pages {

void renderAboutPage(core::Canvas& canvas, const AboutPageView& view)
{
    ui::drawListPageFrame(canvas);
    ui::drawTopBar(canvas, "ABOUT", true);
    canvas.fillRect(8, 30, 304, 126, ui::kBgPanel1);
    canvas.strokeRect(8, 30, 304, 126, ui::kDivider, 1);
    ui::drawText(canvas, "LOFIBOX", 18, 42, ui::kTextPrimary, 1);
    ui::drawText(canvas, "ZERO", 18, 58, ui::kTextPrimary, 2);
    ui::drawText(canvas, "VERSION", 120, 40, ui::kTextMuted, 1);
    ui::drawText(canvas, view.version, 190, 40, ui::kTextPrimary, 1);
    ui::drawText(canvas, "STORAGE", 120, 58, ui::kTextMuted, 1);
    ui::drawText(canvas, view.storage, 190, 58, ui::kTextPrimary, 1);
    ui::drawText(canvas, "MODEL", 120, 76, ui::kTextMuted, 1);
    ui::drawText(canvas, "CARDPUTER ZERO", 176, 76, ui::kTextPrimary, 1);
}

} // namespace lofibox::app::pages
