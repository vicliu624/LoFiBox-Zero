// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/host_runtime_service_providers.h"

#include <cassert>

int main()
{
    auto context = lofibox::platform::host::createHostRuntimeServiceContext();
    assert(context.cache != nullptr);

    const auto connectivity = lofibox::platform::host::createHostConnectivityServices(context);
    assert(connectivity.provider != nullptr);
    assert(context.connectivity.provider == connectivity.provider);

    const auto metadata = lofibox::platform::host::createHostMetadataServices(context);
    assert(metadata.metadata_provider != nullptr);
    assert(metadata.track_identity_provider != nullptr);
    assert(metadata.artwork_provider != nullptr);
    assert(metadata.lyrics_provider != nullptr);
    assert(metadata.tag_writer != nullptr);

    const auto playback = lofibox::platform::host::createHostPlaybackServices(context);
    assert(playback.audio_backend != nullptr);

    const auto remote = lofibox::platform::host::createHostRemoteMediaServices(context);
    assert(remote.remote_source_provider != nullptr);
    assert(remote.remote_catalog_provider != nullptr);
    assert(remote.remote_stream_resolver != nullptr);
    return 0;
}
