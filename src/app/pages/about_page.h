// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "core/canvas.h"

namespace lofibox::app::pages {

struct AboutPageView {
    std::string version{};
    std::string storage{};
};

void renderAboutPage(core::Canvas& canvas, const AboutPageView& view);

} // namespace lofibox::app::pages
