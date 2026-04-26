// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "app/runtime_services.h"
#include "platform/host/runtime_enrichment_clients.h"
#include "platform/host/runtime_host_internal.h"

namespace lofibox::platform::host::runtime_detail {

struct MetadataEnrichmentResult {
    MusicBrainzLookupResult online{};
    app::TrackIdentity identity{};
    bool attempted{false};
    std::string skipped_reason{};
};

class MetadataEnrichmentOrchestrator {
public:
    MetadataEnrichmentOrchestrator(
        std::shared_ptr<SharedRuntimeCache> cache,
        std::shared_ptr<app::ConnectivityProvider> connectivity,
        std::shared_ptr<app::TrackIdentityProvider> track_identity_provider);

    [[nodiscard]] MetadataEnrichmentResult resolve(
        const std::filesystem::path& path,
        const app::TrackMetadata& seed_metadata,
        bool online_attempted) const;

private:
    std::shared_ptr<SharedRuntimeCache> cache_{};
    std::shared_ptr<app::ConnectivityProvider> connectivity_{};
    std::shared_ptr<app::TrackIdentityProvider> track_identity_provider_{};
    MusicBrainzMetadataClient musicbrainz_{};
};

} // namespace lofibox::platform::host::runtime_detail
