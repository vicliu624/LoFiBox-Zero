// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include "app/runtime_services.h"
#include "platform/host/runtime_host_internal.h"

namespace lofibox::platform::host {

std::shared_ptr<app::MetadataProvider> createHostMetadataProvider(
    std::shared_ptr<runtime_detail::SharedRuntimeCache> cache,
    std::shared_ptr<app::ConnectivityProvider> connectivity,
    std::shared_ptr<app::TagWriter> tag_writer,
    std::shared_ptr<app::TrackIdentityProvider> track_identity_provider);
std::shared_ptr<app::TrackIdentityProvider> createHostTrackIdentityProvider(
    std::shared_ptr<runtime_detail::SharedRuntimeCache> cache,
    std::shared_ptr<app::ConnectivityProvider> connectivity);
std::shared_ptr<app::ArtworkProvider> createHostArtworkProvider(
    std::shared_ptr<runtime_detail::SharedRuntimeCache> cache,
    std::shared_ptr<app::ConnectivityProvider> connectivity,
    std::shared_ptr<app::TagWriter> tag_writer);
std::shared_ptr<app::LyricsProvider> createHostLyricsProvider(
    std::shared_ptr<runtime_detail::SharedRuntimeCache> cache,
    std::shared_ptr<app::ConnectivityProvider> connectivity,
    std::shared_ptr<app::TagWriter> tag_writer,
    std::shared_ptr<app::TrackIdentityProvider> track_identity_provider);
std::shared_ptr<app::TagWriter> createHostTagWriter();
std::shared_ptr<app::AudioPlaybackBackend> createHostAudioPlaybackBackend();
std::shared_ptr<app::ConnectivityProvider> createHostConnectivityProvider();
std::shared_ptr<app::RemoteSourceProvider> createHostRemoteSourceProvider();
std::shared_ptr<app::RemoteCatalogProvider> createHostRemoteCatalogProvider();
std::shared_ptr<app::RemoteStreamResolver> createHostRemoteStreamResolver();

} // namespace lofibox::platform::host
