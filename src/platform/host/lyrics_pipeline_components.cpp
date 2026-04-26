// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/lyrics_pipeline_components.h"

namespace lofibox::platform::host::runtime_detail {

bool LyricsCache::loadCurrentCachedLyrics(const SharedRuntimeCache& cache, std::string_view key, CacheEntry& entry) const
{
    if (!entry.online_lyrics_attempted && entry.lyrics_lookup_version >= kLyricsLookupVersion && fs::exists(lyricsCachePath(cache, key))) {
        entry.lyrics = loadLyricsCache(cache, key);
    }
    return entry.lyrics.plain || entry.lyrics.synced;
}

void LyricsCache::storeLookupAttempt(const SharedRuntimeCache& cache, std::string_view key, CacheEntry& entry) const
{
    entry.online_lyrics_attempted = true;
    entry.lyrics_lookup_version = kLyricsLookupVersion;
    storeMetadataCache(cache, key, entry);
}

void LyricsCache::storeHit(const SharedRuntimeCache& cache, std::string_view key, CacheEntry& entry) const
{
    storeLyricsCache(cache, key, entry.lyrics);
}

bool LyricsWritebackPolicy::shouldWrite(const app::TrackLyrics& lyrics) const noexcept
{
    return lyrics.source == "LRCLIB";
}

bool LyricsWritebackPolicy::shouldWrite(const app::TrackLyrics& lyrics, const app::TrackIdentity& identity) const noexcept
{
    return authority_policy_.allowLyricsWriteback(identity, lyrics);
}

bool LyricsWritebackPolicy::shouldOnlyFillMissing(bool embedded_lyrics_rejected) const noexcept
{
    return !embedded_lyrics_rejected;
}

} // namespace lofibox::platform::host::runtime_detail
