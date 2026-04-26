// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include "app/runtime_services.h"
#include "platform/host/runtime_host_internal.h"

namespace lofibox::platform::host {

struct HostRuntimeServiceContext {
    std::shared_ptr<runtime_detail::SharedRuntimeCache> cache{};
    app::ConnectivityServices connectivity{};
    std::shared_ptr<app::TagWriter> tag_writer{};
    std::shared_ptr<app::TrackIdentityProvider> track_identity_provider{};
};

[[nodiscard]] HostRuntimeServiceContext createHostRuntimeServiceContext();
[[nodiscard]] app::ConnectivityServices createHostConnectivityServices(HostRuntimeServiceContext& context);
[[nodiscard]] app::MetadataServices createHostMetadataServices(HostRuntimeServiceContext& context);
[[nodiscard]] app::PlaybackServices createHostPlaybackServices(HostRuntimeServiceContext& context);
[[nodiscard]] app::RemoteMediaServices createHostRemoteMediaServices(HostRuntimeServiceContext& context);

} // namespace lofibox::platform::host
