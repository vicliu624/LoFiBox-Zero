// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <string_view>

#include "app/runtime_services.h"
#include "metadata/match_confidence_guard.h"
#include "platform/host/runtime_host_internal.h"

namespace lofibox::platform::host::runtime_detail {

inline constexpr int kLyricsLookupVersion = 5;

class LyricsCache {
public:
    [[nodiscard]] bool loadCurrentCachedLyrics(const SharedRuntimeCache& cache, std::string_view key, CacheEntry& entry) const;
    void storeLookupAttempt(const SharedRuntimeCache& cache, std::string_view key, CacheEntry& entry) const;
    void storeHit(const SharedRuntimeCache& cache, std::string_view key, CacheEntry& entry) const;
};

class LyricsWritebackPolicy {
public:
    [[nodiscard]] bool shouldWrite(const app::TrackLyrics& lyrics) const noexcept;
    [[nodiscard]] bool shouldWrite(const app::TrackLyrics& lyrics, const app::TrackIdentity& identity) const noexcept;
    [[nodiscard]] bool shouldOnlyFillMissing(bool embedded_lyrics_rejected) const noexcept;

private:
    ::lofibox::metadata::MatchConfidenceGuard match_guard_{};
};

} // namespace lofibox::platform::host::runtime_detail
