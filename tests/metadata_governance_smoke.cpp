// SPDX-License-Identifier: GPL-3.0-or-later

#include "metadata/metadata_governance.h"

#include <cassert>

int main()
{
    lofibox::metadata::MetadataGovernanceService service{};
    lofibox::app::TrackIdentity identity{};
    identity.found = true;
    identity.recording_mbid = "recording-1";
    assert(service.rememberFingerprint({"fingerprint-1", "song.mp3", identity}));
    assert(service.lookupFingerprint("fingerprint-1"));

    lofibox::app::TrackMetadata existing{};
    existing.title = "Existing";
    existing.artist = "Artist";
    lofibox::app::TrackMetadata proposed{};
    proposed.title = "Proposed";
    proposed.artist = "Artist";
    const auto conflicts = service.conflicts(existing, proposed, "MusicBrainz", 0.9);
    assert(conflicts.size() == 1);
    assert(conflicts.front().field == "title");

    const auto artwork = service.chooseArtwork(
        lofibox::metadata::ArtworkProvenance{lofibox::metadata::ArtworkSourceKind::Embedded, "embedded.png"},
        lofibox::metadata::ArtworkProvenance{lofibox::metadata::ArtworkSourceKind::Directory, "cover.png"},
        lofibox::metadata::ArtworkProvenance{lofibox::metadata::ArtworkSourceKind::Online, "online.png"},
        std::nullopt);
    assert(artwork.source == lofibox::metadata::ArtworkSourceKind::Embedded);
    assert(artwork.explanation.find("Embedded") != std::string::npos);

    assert(service.detectTagFormat("a.mp3") == lofibox::metadata::TagFormat::Id3);
    assert(service.detectTagFormat("a.flac") == lofibox::metadata::TagFormat::FlacVorbis);
    assert(service.detectTagFormat("a.ogg") == lofibox::metadata::TagFormat::OggVorbis);
    assert(service.detectTagFormat("a.m4a") == lofibox::metadata::TagFormat::Mp4);
    assert(service.writebackSupported(lofibox::metadata::TagFormat::Mp4));
    return 0;
}
