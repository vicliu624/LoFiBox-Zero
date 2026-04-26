// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include "metadata/match_confidence_guard.h"
#include "metadata/metadata_merge_policy.h"

int main()
{
    lofibox::app::TrackRecord track{};
    track.title = "Original Title";
    track.artist = "Original Artist";
    track.album = "UNKNOWN";
    track.duration_seconds = 0;

    lofibox::app::TrackMetadata weak_metadata{};
    weak_metadata.title = "Wrong Online Title";
    weak_metadata.artist = "Broken Online Match";
    weak_metadata.album = "Filled Album";
    weak_metadata.duration_seconds = 241;

    lofibox::metadata::MetadataMergePolicy merge_policy{};
    merge_policy.mergeIntoTrack(track, weak_metadata);

    if (track.title != "Original Title" || track.artist != "Original Artist") {
        std::cerr << "Expected merge policy to preserve existing non-placeholder identity text.\n";
        return 1;
    }
    if (track.album != "Filled Album" || track.duration_seconds != 241) {
        std::cerr << "Expected merge policy to fill placeholders and duration.\n";
        return 1;
    }

    lofibox::metadata::MatchConfidenceGuard guard{};
    lofibox::app::TrackIdentity weak_identity{};
    weak_identity.found = true;
    weak_identity.confidence = 0.25;
    weak_identity.audio_fingerprint_verified = false;
    lofibox::app::TrackLyrics lyrics{};
    lyrics.source = "LRCLIB";
    lyrics.plain = "lyrics";
    if (guard.acceptsAuthoritativeLyrics(lyrics, weak_identity)) {
        std::cerr << "Expected weak identity to reject authoritative writeback.\n";
        return 1;
    }

    lofibox::app::TrackIdentity strong_identity = weak_identity;
    strong_identity.confidence = 0.92;
    if (!guard.acceptsAuthoritativeLyrics(lyrics, strong_identity)) {
        std::cerr << "Expected strong identity to accept authoritative writeback.\n";
        return 1;
    }

    return 0;
}
