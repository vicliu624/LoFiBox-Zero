// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/runtime_services.h"

namespace lofibox::metadata {

enum class EnrichmentEvidenceKind {
    EmbeddedTags,
    FingerprintVerifiedOnline,
    TrustedOnline,
    WeakOnline,
    FilenameFallback,
};

struct EnrichmentEvidence {
    EnrichmentEvidenceKind kind{EnrichmentEvidenceKind::WeakOnline};
    double confidence{0.0};
    bool duration_compatible{true};
    bool text_compatible{true};
};

enum class EnrichmentDecisionKind {
    AcceptAuthoritative,
    FillMissingOnly,
    Reject,
};

class EnrichmentAuthorityPolicy {
public:
    [[nodiscard]] EnrichmentDecisionKind decideMetadataMerge(const EnrichmentEvidence& evidence) const noexcept;
    [[nodiscard]] bool allowLyricsWriteback(const app::TrackIdentity& identity, const app::TrackLyrics& lyrics) const noexcept;
    [[nodiscard]] bool allowArtworkWriteback(const EnrichmentEvidence& evidence) const noexcept;
};

} // namespace lofibox::metadata
