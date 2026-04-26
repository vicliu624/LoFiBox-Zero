// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/host_runtime_service_providers.h"

#include <memory>
#include <utility>

#include "platform/host/runtime_provider_factories.h"
#include "platform/host/xdg_remote_profile_store.h"

namespace lofibox::platform::host {

HostRuntimeServiceContext createHostRuntimeServiceContext()
{
    HostRuntimeServiceContext context{};
    context.cache = std::make_shared<runtime_detail::SharedRuntimeCache>();
    return context;
}

app::ConnectivityServices createHostConnectivityServices(HostRuntimeServiceContext& context)
{
    context.connectivity.provider = createHostConnectivityProvider();
    return context.connectivity;
}

app::MetadataServices createHostMetadataServices(HostRuntimeServiceContext& context)
{
    if (!context.connectivity.provider) {
        context.connectivity = createHostConnectivityServices(context);
    }
    if (!context.tag_writer) {
        context.tag_writer = createHostTagWriter();
    }
    if (!context.track_identity_provider) {
        context.track_identity_provider = createHostTrackIdentityProvider(context.cache, context.connectivity.provider);
    }

    app::MetadataServices metadata_group{};
    metadata_group.metadata_provider = createHostMetadataProvider(
        context.cache,
        context.connectivity.provider,
        context.tag_writer,
        context.track_identity_provider);
    metadata_group.track_identity_provider = context.track_identity_provider;
    metadata_group.artwork_provider = createHostArtworkProvider(
        context.cache,
        context.connectivity.provider,
        context.tag_writer);
    metadata_group.lyrics_provider = createHostLyricsProvider(
        context.cache,
        context.connectivity.provider,
        context.tag_writer,
        context.track_identity_provider);
    metadata_group.tag_writer = context.tag_writer;
    return metadata_group;
}

app::PlaybackServices createHostPlaybackServices(HostRuntimeServiceContext&)
{
    app::PlaybackServices playback_group{};
    playback_group.audio_backend = createHostAudioPlaybackBackend();
    return playback_group;
}

app::RemoteMediaServices createHostRemoteMediaServices(HostRuntimeServiceContext&)
{
    app::RemoteMediaServices remote_group{};
    remote_group.remote_source_provider = createHostRemoteSourceProvider();
    remote_group.remote_catalog_provider = createHostRemoteCatalogProvider();
    remote_group.remote_stream_resolver = createHostRemoteStreamResolver();
    remote_group.remote_profile_store = std::make_shared<XdgRemoteProfileStore>();
    return remote_group;
}

} // namespace lofibox::platform::host
