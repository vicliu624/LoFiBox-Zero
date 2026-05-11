// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>
#include <string_view>

#include "platform/host/png_canvas_loader.h"

#ifndef LOFIBOX_ASSET_DIR
#define LOFIBOX_ASSET_DIR "assets"
#endif

int main()
{
    const auto icon_dir = std::filesystem::path{LOFIBOX_ASSET_DIR}
        / "ui"
        / "icons"
        / "legacy-lofibox";

    constexpr std::string_view icons[] = {
        "About.png",
        "Equalizer.png",
        "Groove.png",
        "Library.png",
        "Music.png",
        "NowPlaying.png",
        "Playlists.png",
        "Settings.png",
    };

    for (const auto icon : icons) {
        const auto path = icon_dir / icon;
        const auto canvas = lofibox::platform::host::loadPngCanvas(path);
        if (!canvas) {
            std::cerr << "Expected legacy UI icon to load: " << path << ".\n";
            return 1;
        }
        if (canvas->width() != 164 || canvas->height() != 164) {
            std::cerr << "Expected legacy UI icon " << path << " to be 164x164, got "
                      << canvas->width() << "x" << canvas->height() << ".\n";
            return 1;
        }
    }

    return 0;
}
