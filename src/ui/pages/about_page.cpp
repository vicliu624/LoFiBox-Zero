// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/pages/about_page.h"

#include "ui/ui_primitives.h"
#include "ui/ui_theme.h"

namespace lofibox::ui::pages {

void renderAboutPage(core::Canvas& canvas, const AboutPageView& view)
{
    ::lofibox::ui::drawListPageFrame(canvas);
    ::lofibox::ui::drawTopBar(canvas, "ABOUT", true);
    canvas.fillRect(8, 30, 304, 126, ::lofibox::ui::kBgPanel1);
    canvas.strokeRect(8, 30, 304, 126, ::lofibox::ui::kDivider, 1);
    ::lofibox::ui::drawText(canvas, "LOFIBOX", 18, 42, ::lofibox::ui::kTextPrimary, 1);
    ::lofibox::ui::drawText(canvas, "ZERO", 18, 58, ::lofibox::ui::kTextPrimary, 2);
    ::lofibox::ui::drawText(canvas, "VERSION", 120, 40, ::lofibox::ui::kTextMuted, 1);
    ::lofibox::ui::drawText(canvas, view.version, 190, 40, ::lofibox::ui::kTextPrimary, 1);
    ::lofibox::ui::drawText(canvas, "STORAGE", 120, 58, ::lofibox::ui::kTextMuted, 1);
    ::lofibox::ui::drawText(canvas, view.storage, 190, 58, ::lofibox::ui::kTextPrimary, 1);
    ::lofibox::ui::drawText(canvas, "MODEL", 120, 76, ::lofibox::ui::kTextMuted, 1);
    ::lofibox::ui::drawText(canvas, "CARDPUTER ZERO", 176, 76, ::lofibox::ui::kTextPrimary, 1);
}

} // namespace lofibox::ui::pages
