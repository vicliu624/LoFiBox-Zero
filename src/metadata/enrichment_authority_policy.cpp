// SPDX-License-Identifier: GPL-3.0-or-later

#include "metadata/enrichment_authority_policy.h"

#include "metadata/match_confidence_guard.h"

namespace lofibox::metadata {

EnrichmentDecisionKind EnrichmentAuthorityPolicy::decideMetadataMerge(const EnrichmentEvidence& evidence) const noexcept
{
    if (!evidence.duration_compatible || !evidence.text_compatible) {
        return EnrichmentDecisionKind::Reject;
    }
    switch (evidence.kind) {
    case EnrichmentEvidenceKind::EmbeddedTags:
    case EnrichmentEvidenceKind::FingerprintVerifiedOnline:
        return EnrichmentDecisionKind::AcceptAuthoritative;
    case EnrichmentEvidenceKind::TrustedOnline:
        return evidence.confidence >= 0.86 ? EnrichmentDecisionKind::AcceptAuthoritative : EnrichmentDecisionKind::FillMissingOnly;
    case EnrichmentEvidenceKind::WeakOnline:
        return evidence.confidence >= 0.72 ? EnrichmentDecisionKind::FillMissingOnly : EnrichmentDecisionKind::Reject;
    case EnrichmentEvidenceKind::FilenameFallback:
        return EnrichmentDecisionKind::FillMissingOnly;
    }
    return EnrichmentDecisionKind::Reject;
}

bool EnrichmentAuthorityPolicy::allowLyricsWriteback(const app::TrackIdentity& identity, const app::TrackLyrics& lyrics) const noexcept
{
    return MatchConfidenceGuard{}.acceptsAuthoritativeLyrics(lyrics, identity);
}

bool EnrichmentAuthorityPolicy::allowArtworkWriteback(const EnrichmentEvidence& evidence) const noexcept
{
    const auto decision = decideMetadataMerge(evidence);
    return decision == EnrichmentDecisionKind::AcceptAuthoritative || decision == EnrichmentDecisionKind::FillMissingOnly;
}

} // namespace lofibox::metadata
