// SPDX-License-Identifier: GPL-3.0-or-later

#include "cache/cache_manager.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "lofibox-cache-manager-smoke";
    std::error_code ec{};
    std::filesystem::remove_all(root, ec);

    lofibox::cache::CacheManager manager{root};
    manager.setPolicy(lofibox::cache::CacheBucket::RemoteDirectory, lofibox::cache::CachePolicy{1024 * 1024, std::chrono::hours{24}});

    assert(manager.rememberRemoteDirectory("emby-home", "artists", "[artist catalog]"));
    assert(manager.remoteDirectory("emby-home", "artists").value() == "[artist catalog]");

    assert(manager.rememberStationList("radio", "station-a\nstation-b"));
    assert(manager.stationList("radio").value().find("station-b") != std::string::npos);

    assert(manager.rememberRecentBrowse("emby-home", "track-1", "Track One"));
    assert(!manager.recentBrowseEntries("emby-home").empty());

    lofibox::cache::OfflineSaveRequest track{};
    track.source_id = "emby-home";
    track.item_id = "track-1";
    track.title = "Track One";
    track.album = "Album One";
    track.extension = ".mp3";
    assert(manager.saveOfflineTrack(track, std::vector<std::uint8_t>{1, 2, 3, 4}));
    const auto offline_path = manager.offlineTrackPath("emby-home", "track-1");
    assert(offline_path);
    assert(std::filesystem::exists(*offline_path));

    auto album_plan = manager.planAlbumSync("emby-home", "Album One", {track});
    assert(album_plan.scope == "album:Album One");
    assert(album_plan.items.front().album == "Album One");

    auto playlist_plan = manager.planPlaylistSync("emby-home", "Favorites", {track});
    assert(playlist_plan.scope == "playlist:Favorites");
    assert(playlist_plan.items.front().playlist == "Favorites");

    const auto usage = manager.usage();
    assert(usage.entry_count >= 4);
    assert(usage.total_bytes > 0);

    std::filesystem::remove_all(root, ec);
    return 0;
}
