// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_services_factory.h"

#include <memory>

#include "platform/host/runtime_provider_factories.h"

namespace lofibox::platform::host {

app::RuntimeServices createHostRuntimeServices()
{
    auto cache = std::make_shared<runtime_detail::SharedRuntimeCache>();
    auto connectivity = createHostConnectivityProvider();
    auto tag_writer = createHostTagWriter();
    auto track_identity = createHostTrackIdentityProvider(cache, connectivity);

    return app::withNullRuntimeServices(app::RuntimeServices{
        createHostMetadataProvider(cache, connectivity, tag_writer, track_identity),
        track_identity,
        createHostArtworkProvider(cache, connectivity, tag_writer),
        createHostAudioPlaybackBackend(),
        createHostLyricsProvider(cache, connectivity, tag_writer, track_identity),
        tag_writer,
        connectivity,
        createHostRemoteSourceProvider(),
        createHostRemoteCatalogProvider(),
        createHostRemoteStreamResolver()});
}

} // namespace lofibox::platform::host
