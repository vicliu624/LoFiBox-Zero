// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "app/remote_media_services.h"
#include "app/runtime_services.h"

namespace lofibox::app {

[[nodiscard]] std::string remoteMediaCacheKey(const RemoteServerProfile& profile, std::string_view item_id);
[[nodiscard]] std::string remoteDirectoryCacheKey(const RemoteServerProfile& profile, const RemoteCatalogNode& parent);
[[nodiscard]] std::string remoteBrowseCachePayload(const std::vector<RemoteCatalogNode>& nodes);
[[nodiscard]] std::vector<RemoteCatalogNode> parseRemoteBrowseCachePayload(std::string_view text);
[[nodiscard]] std::string serializeRemoteTrackCache(const RemoteTrack& track);
[[nodiscard]] RemoteTrack parseRemoteTrackCache(std::string_view text);
[[nodiscard]] RemoteTrack mergeRemoteTrackCache(RemoteTrack current, const RemoteTrack& cached);
[[nodiscard]] RemoteTrack remoteTrackFromCatalogNode(const RemoteCatalogNode& node);
[[nodiscard]] bool remoteTrackHasLocalCacheableFacts(const RemoteTrack& track) noexcept;
[[nodiscard]] bool remoteTrackNeedsLocalMetadataGovernance(const RemoteTrack& track) noexcept;
[[nodiscard]] TrackMetadata remoteTrackMetadataSeed(const RemoteTrack& track);
[[nodiscard]] std::filesystem::path remoteTrackSyntheticLookupPath(const RemoteServerProfile& profile, const RemoteTrack& track);
[[nodiscard]] RemoteTrack mergeRemoteGovernedFacts(RemoteTrack current, const TrackMetadata& metadata, const TrackLyrics& lyrics);

} // namespace lofibox::app
