// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/legacy_asset_loader.h"

#include <filesystem>
#include <string>

#if defined(_MSC_VER)
#include <cstdlib>
#else
#include <cstdlib>
#endif

#include "platform/host/png_canvas_loader.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host {
namespace {

std::string configuredAssetRoot()
{
#if defined(_MSC_VER)
    char* buffer = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&buffer, &size, "LOFIBOX_ASSET_DIR") == 0 && buffer != nullptr) {
        std::string value{buffer};
        std::free(buffer);
        return value;
    }
    return {};
#else
    if (const char* configured_root = std::getenv("LOFIBOX_ASSET_DIR")) {
        return configured_root;
    }
    return {};
#endif
}

bool hasLegacyIconSet(const fs::path& asset_root)
{
    std::error_code ec{};
    const fs::path base = asset_root / "ui" / "icons" / "legacy-lofibox";
    return fs::exists(base / "logo.png", ec) && !ec;
}

fs::path defaultAssetRoot()
{
    const auto configured_root = configuredAssetRoot();
    if (!configured_root.empty()) {
        return fs::path{configured_root};
    }

#if defined(LOFIBOX_SYSTEM_ASSET_DIR)
    const fs::path system_root{LOFIBOX_SYSTEM_ASSET_DIR};
    if (hasLegacyIconSet(system_root)) {
        return system_root;
    }
#endif

#if defined(LOFIBOX_ASSET_DIR)
    const fs::path source_root{LOFIBOX_ASSET_DIR};
    if (hasLegacyIconSet(source_root)) {
        return source_root;
    }
#endif

    return fs::path{"assets"};
}

} // namespace

app::AppAssets loadLegacyAssets()
{
    app::AppAssets assets{};

    const fs::path asset_root = defaultAssetRoot();
    const fs::path base = asset_root / "ui" / "icons" / "legacy-lofibox";
    assets.logo = loadPngCanvas(base / "logo.png");
    assets.music_icon = loadPngCanvas(base / "Music.png");
    assets.library_icon = loadPngCanvas(base / "Library.png");
    assets.playlists_icon = loadPngCanvas(base / "Playlists.png");
    assets.now_playing_icon = loadPngCanvas(base / "NowPlaying.png");
    assets.equalizer_icon = loadPngCanvas(base / "Equalizer.png");
    assets.settings_icon = loadPngCanvas(base / "Settings.png");

    return assets;
}

} // namespace lofibox::platform::host
