// SPDX-License-Identifier: GPL-3.0-or-later

#include "metadata/enrichment_authority_policy.h"

#include <cassert>

int main()
{
    lofibox::metadata::EnrichmentAuthorityPolicy policy{};

    assert(policy.decideMetadataMerge({
        lofibox::metadata::EnrichmentEvidenceKind::FingerprintVerifiedOnline,
        0.92,
        true,
        true}) == lofibox::metadata::EnrichmentDecisionKind::AcceptAuthoritative);

    assert(policy.decideMetadataMerge({
        lofibox::metadata::EnrichmentEvidenceKind::WeakOnline,
        0.60,
        true,
        true}) == lofibox::metadata::EnrichmentDecisionKind::Reject);

    assert(policy.decideMetadataMerge({
        lofibox::metadata::EnrichmentEvidenceKind::TrustedOnline,
        0.90,
        false,
        true}) == lofibox::metadata::EnrichmentDecisionKind::Reject);

    assert(policy.decideMetadataMerge({
        lofibox::metadata::EnrichmentEvidenceKind::FilenameFallback,
        0.0,
        true,
        true}) == lofibox::metadata::EnrichmentDecisionKind::FillMissingOnly);

    lofibox::app::TrackIdentity identity{};
    identity.found = true;
    identity.confidence = 0.91;
    identity.audio_fingerprint_verified = true;
    lofibox::app::TrackLyrics lyrics{};
    lyrics.synced = "[00:01.00]hello";
    lyrics.source = "LRCLIB";
    assert(policy.allowLyricsWriteback(identity, lyrics));

    identity.confidence = 0.3;
    identity.audio_fingerprint_verified = false;
    assert(!policy.allowLyricsWriteback(identity, lyrics));
    return 0;
}
