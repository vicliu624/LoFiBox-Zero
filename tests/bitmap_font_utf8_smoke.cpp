// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstddef>
#include <iostream>

#include "core/bitmap_font.h"
#include "core/canvas.h"
#include "core/color.h"

int main()
{
    lofibox::core::Canvas canvas{160, 40};
    const auto background = lofibox::core::rgba(0, 0, 0);
    canvas.clear(background);

    lofibox::core::bitmap_font::drawText(
        canvas,
        "测试中文",
        4,
        4,
        lofibox::core::rgba(255, 255, 255),
        1);

    std::size_t changed = 0;
    for (const auto& pixel : canvas.pixels()) {
        if (pixel != background) {
            ++changed;
        }
    }

    if (changed == 0) {
        std::cerr << "Expected UTF-8 text rendering to draw visible pixels.\n";
        return 1;
    }

    return 0;
}
