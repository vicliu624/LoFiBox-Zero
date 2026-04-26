// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_services_factory.h"

#include <memory>
#include <utility>

#include "platform/host/runtime_provider_factories.h"

namespace lofibox::platform::host {

app::RuntimeServices createHostRuntimeServices()
{
    auto cache = std::make_shared<runtime_detail::SharedRuntimeCache>();
    auto connectivity = createHostConnectivityProvider();
    auto tag_writer = createHostTagWriter();
    auto track_identity = createHostTrackIdentityProvider(cache, connectivity);

    app::RuntimeServices services{};
    services.connectivity.provider = connectivity;
    services.metadata.metadata_provider = createHostMetadataProvider(cache, connectivity, tag_writer, track_identity);
    services.metadata.track_identity_provider = track_identity;
    services.metadata.artwork_provider = createHostArtworkProvider(cache, connectivity, tag_writer);
    services.metadata.lyrics_provider = createHostLyricsProvider(cache, connectivity, tag_writer, track_identity);
    services.metadata.tag_writer = tag_writer;
    services.playback.audio_backend = createHostAudioPlaybackBackend();
    services.remote.remote_source_provider = createHostRemoteSourceProvider();
    services.remote.remote_catalog_provider = createHostRemoteCatalogProvider();
    services.remote.remote_stream_resolver = createHostRemoteStreamResolver();
    return app::withNullRuntimeServices(std::move(services));
}

} // namespace lofibox::platform::host
