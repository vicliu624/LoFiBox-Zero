// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>

#include "core/color.h"
#include "platform/host/png_canvas_loader.h"
#include "ui/ui_primitives.h"

#ifndef LOFIBOX_ASSET_DIR
#define LOFIBOX_ASSET_DIR "assets"
#endif

namespace {

bool isTransparent(const lofibox::core::Color& color)
{
    return color.a <= 4;
}

} // namespace

int main()
{
    const auto logo_path = std::filesystem::path{LOFIBOX_ASSET_DIR}
        / "ui"
        / "icons"
        / "legacy-lofibox"
        / "logo.png";

    const auto logo = lofibox::platform::host::loadPngCanvas(logo_path);
    if (!logo) {
        std::cerr << "Expected runtime logo asset to load from " << logo_path << ".\n";
        return 1;
    }

    if (logo->width() <= 0 || logo->height() <= 0) {
        std::cerr << "Expected runtime logo asset to have positive dimensions.\n";
        return 1;
    }

    const lofibox::core::Color corners[] = {
        logo->pixel(0, 0),
        logo->pixel(logo->width() - 1, 0),
        logo->pixel(0, logo->height() - 1),
        logo->pixel(logo->width() - 1, logo->height() - 1),
    };

    for (const auto& corner : corners) {
        if (!isTransparent(corner)) {
            std::cerr << "Expected runtime logo corners to preserve alpha transparency or only near-transparent antialiasing.\n";
            return 1;
        }
    }

    std::size_t transparent_pixels = 0;
    std::size_t visible_foreground_pixels = 0;
    for (const auto& pixel : logo->pixels()) {
        if (pixel.a == 0) {
            ++transparent_pixels;
        }
        if (pixel.a > 32) {
            ++visible_foreground_pixels;
        }
    }

    if (transparent_pixels < logo->pixels().size() / 8) {
        std::cerr << "Expected the logo asset to contain a substantial transparent background.\n";
        return 1;
    }

    if (visible_foreground_pixels < logo->pixels().size() / 4) {
        std::cerr << "Expected the logo asset to retain visible foreground artwork.\n";
        return 1;
    }

    lofibox::core::Canvas canvas{96, 96};
    const auto background = lofibox::core::rgba(7, 11, 19, 255);
    canvas.clear(background);
    lofibox::ui::blitScaledCanvas(canvas, *logo, 8, 8, 72, 72, 255);

    if (canvas.pixel(8, 8) != background) {
        std::cerr << "Expected scaled logo blit to preserve destination pixels behind transparent logo corners.\n";
        return 1;
    }

    if (canvas.pixel(44, 44) == background) {
        std::cerr << "Expected scaled logo blit to draw foreground artwork.\n";
        return 1;
    }

    return 0;
}
