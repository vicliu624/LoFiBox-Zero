// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/lyrics_pipeline_components.h"
#include "platform/host/runtime_host_internal.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <string>

int main()
{
    lofibox::platform::host::runtime_detail::LyricsWritebackPolicy policy{};
    lofibox::app::TrackLyrics lyrics{};
    lyrics.source = "LRCLIB";
    lyrics.synced = "[00:01.00]hello";

    lofibox::app::TrackIdentity trusted{};
    trusted.found = true;
    trusted.confidence = 0.90;
    assert(policy.shouldWrite(lyrics, trusted));

    lofibox::app::TrackIdentity weak{};
    weak.found = true;
    weak.confidence = 0.40;
    assert(!policy.shouldWrite(lyrics, weak));

    lyrics.source = "lyrics.ovh";
    trusted.confidence = 0.95;
    assert(!policy.shouldWrite(lyrics, trusted));

    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::string key = "lyrics-cache-hit-smoke-" + std::to_string(now);
    lofibox::platform::host::runtime_detail::SharedRuntimeCache cache{};
    lofibox::platform::host::runtime_detail::CacheEntry entry{};
    entry.online_lyrics_attempted = true;
    entry.lyrics_lookup_version = lofibox::platform::host::runtime_detail::kLyricsLookupVersion;

    lofibox::app::TrackLyrics cached{};
    cached.plain = "cached lyric survives online-attempt state";
    cached.source = "LRCLIB";
    lofibox::platform::host::runtime_detail::storeLyricsCache(cache, key, cached);

    lofibox::platform::host::runtime_detail::LyricsCache lyrics_cache{};
    assert(lyrics_cache.loadCurrentCachedLyrics(cache, key, entry));
    assert(entry.lyrics.plain && *entry.lyrics.plain == *cached.plain);
    std::error_code ec{};
    std::filesystem::remove(lofibox::platform::host::runtime_detail::lyricsCachePath(cache, key), ec);
    return 0;
}
