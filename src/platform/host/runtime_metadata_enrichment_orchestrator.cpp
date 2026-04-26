// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_metadata_enrichment_orchestrator.h"

#include <utility>

namespace lofibox::platform::host::runtime_detail {

MetadataEnrichmentOrchestrator::MetadataEnrichmentOrchestrator(
    std::shared_ptr<SharedRuntimeCache> cache,
    std::shared_ptr<app::ConnectivityProvider> connectivity,
    std::shared_ptr<app::TrackIdentityProvider> track_identity_provider)
    : cache_(std::move(cache))
    , connectivity_(std::move(connectivity))
    , track_identity_provider_(std::move(track_identity_provider))
{
}

MetadataEnrichmentResult MetadataEnrichmentOrchestrator::resolve(
    const std::filesystem::path& path,
    const app::TrackMetadata& seed_metadata,
    bool online_attempted) const
{
    MetadataEnrichmentResult result{};
    if (online_attempted) {
        result.skipped_reason = "already attempted";
        return result;
    }
    if (!cache_) {
        result.skipped_reason = "cache unavailable";
        return result;
    }
    const bool identity_available = track_identity_provider_ && track_identity_provider_->available();
    if (!identity_available && !musicbrainz_.available()) {
        result.skipped_reason = "curl unavailable";
        return result;
    }
    if (!connectivity_) {
        result.skipped_reason = "connectivity provider missing";
        return result;
    }
    if (!connectivity_->connected()) {
        result.skipped_reason = "network disconnected";
        return result;
    }

    result.attempted = true;
    if (identity_available) {
        result.identity = track_identity_provider_->resolve(path, seed_metadata);
        if (result.identity.found) {
            result.online.found = true;
            result.online.metadata = result.identity.metadata;
            result.online.recording_mbid = result.identity.recording_mbid;
            result.online.release_mbid = result.identity.release_mbid;
            result.online.release_group_mbid = result.identity.release_group_mbid;
            result.online.confidence = result.identity.confidence;
            return result;
        }
    }
    if (musicbrainz_.available()) {
        result.online = musicbrainz_.lookup(path, *cache_, seed_metadata);
    }
    return result;
}

} // namespace lofibox::platform::host::runtime_detail
