// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "app/remote_media_services.h"

namespace lofibox::remote {

enum class RemoteProviderCapability {
    Probe,
    SearchTracks,
    RecentTracks,
    ResolveStream,
    BrowseCatalog,
    ReadOnly,
};

enum class RemoteProviderErrorKind {
    None,
    Unsupported,
    InvalidProfile,
    AuthenticationFailed,
    NetworkUnavailable,
    Timeout,
    ServerRejected,
};

struct RemoteProviderError {
    RemoteProviderErrorKind kind{RemoteProviderErrorKind::None};
    std::string message{};
};

struct RemoteProviderManifest {
    app::RemoteServerKind kind{app::RemoteServerKind::Jellyfin};
    std::string type_id{};
    std::string family{};
    std::string display_name{};
    std::vector<RemoteProviderCapability> capabilities{};
};

[[nodiscard]] std::string_view remoteServerKindId(app::RemoteServerKind kind) noexcept;
[[nodiscard]] std::string_view remoteProviderFamily(app::RemoteServerKind kind) noexcept;
[[nodiscard]] bool remoteProviderHasCapability(const RemoteProviderManifest& manifest, RemoteProviderCapability capability) noexcept;
[[nodiscard]] RemoteProviderManifest remoteProviderManifest(app::RemoteServerKind kind);

} // namespace lofibox::remote
