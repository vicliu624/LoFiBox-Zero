// SPDX-License-Identifier: GPL-3.0-or-later

#include "metadata/match_confidence_guard.h"

namespace lofibox::metadata {

bool MatchConfidenceGuard::acceptsIdentity(const app::TrackIdentity& identity) const noexcept
{
    return identity.found && (identity.audio_fingerprint_verified || identity.confidence >= 0.86);
}

bool MatchConfidenceGuard::acceptsAuthoritativeLyrics(const app::TrackLyrics& lyrics, const app::TrackIdentity& identity) const noexcept
{
    if (lyrics.source != "LRCLIB") {
        return false;
    }
    if (!identity.found) {
        return true;
    }
    return acceptsIdentity(identity);
}

} // namespace lofibox::metadata
