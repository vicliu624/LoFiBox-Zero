// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_provider_factories.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include "platform/host/runtime_enrichment_clients.h"
#include "platform/host/runtime_logger.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host {
namespace {
using namespace runtime_detail;

constexpr int kTrackIdentityVersion = 2;

[[nodiscard]] app::TrackIdentity identityFromMusicBrainz(const MusicBrainzLookupResult& result)
{
    app::TrackIdentity identity{};
    if (!result.found) {
        return identity;
    }
    identity.metadata = result.metadata;
    identity.recording_mbid = result.recording_mbid;
    identity.release_mbid = result.release_mbid;
    identity.release_group_mbid = result.release_group_mbid;
    identity.source = "MUSICBRAINZ";
    identity.confidence = result.confidence;
    identity.audio_fingerprint_verified = false;
    identity.found = true;
    return identity;
}

class HostTrackIdentityProvider final : public app::TrackIdentityProvider {
public:
    explicit HostTrackIdentityProvider(
        std::shared_ptr<SharedRuntimeCache> cache,
        std::shared_ptr<app::ConnectivityProvider> connectivity)
        : cache_(std::move(cache))
        , connectivity_(std::move(connectivity))
    {
    }

    [[nodiscard]] bool available() const override
    {
        return acoustid_.available() || musicbrainz_.available();
    }

    [[nodiscard]] std::string displayName() const override
    {
        if (acoustid_.available()) {
            return "Chromaprint -> AcoustID -> MusicBrainz";
        }
        return "MusicBrainz";
    }

    [[nodiscard]] app::TrackIdentity resolve(const fs::path& path, const app::TrackMetadata& seed_metadata = {}) const override
    {
        if (!cache_) {
            return {};
        }

        const auto key = cacheKeyForPath(path);
        auto& entry = cache_->entries[key];
        if (!entry.track_identity_attempted && fs::exists(metadataCachePath(*cache_, key))) {
            entry = loadMetadataCache(*cache_, key);
        }
        const bool cached_identity_is_current = entry.identity.found && entry.track_identity_version >= kTrackIdentityVersion;
        const bool cached_identity_is_strong_enough = entry.identity.audio_fingerprint_verified || !acoustid_.available();
        if (cached_identity_is_current && cached_identity_is_strong_enough) {
            return entry.identity;
        }
        const bool should_retry_for_fingerprint =
            entry.identity.fingerprint.empty() && acoustid_.available();
        if (entry.track_identity_attempted && entry.track_identity_version >= kTrackIdentityVersion && !should_retry_for_fingerprint) {
            return {};
        }
        if (!connectivity_ || !connectivity_->connected()) {
            logRuntime(RuntimeLogLevel::Warn, "identity", "Track identity skipped for " + pathUtf8String(path) + ": network disconnected");
            return {};
        }

        app::TrackIdentity identity{};
        if (acoustid_.available()) {
            identity = acoustid_.lookup(path, seed_metadata);
        } else {
            logRuntime(RuntimeLogLevel::Debug, "identity", "AcoustID unavailable; falling back to MusicBrainz text lookup for " + pathUtf8String(path));
        }

        if (!identity.found && musicbrainz_.available()) {
            identity = identityFromMusicBrainz(musicbrainz_.lookup(path, *cache_, seed_metadata));
            if (identity.found) {
                logRuntime(RuntimeLogLevel::Info, "identity", "MusicBrainz identity hit for " + pathUtf8String(path));
            }
        }

        entry.track_identity_attempted = true;
        entry.track_identity_version = kTrackIdentityVersion;
        if (!identity.fingerprint.empty()) {
            entry.identity.fingerprint = identity.fingerprint;
            if (identity.metadata.duration_seconds) {
                entry.identity.metadata.duration_seconds = identity.metadata.duration_seconds;
            }
        }
        if (identity.found) {
            entry.identity = identity;
            if (!identity.release_mbid.empty()) entry.release_mbid = identity.release_mbid;
            if (!identity.release_group_mbid.empty()) entry.release_group_mbid = identity.release_group_mbid;
            logRuntime(RuntimeLogLevel::Info, "identity", "Track identity resolved by " + identity.source + " for " + pathUtf8String(path));
        } else {
            logRuntime(RuntimeLogLevel::Warn, "identity", "Track identity miss for " + pathUtf8String(path));
        }
        storeMetadataCache(*cache_, key, entry);
        return identity;
    }

    [[nodiscard]] app::TrackIdentity resolveRemoteStream(
        std::string_view stable_cache_key,
        std::string_view stream_uri,
        const fs::path& lookup_path,
        const app::TrackMetadata& seed_metadata = {}) const override
    {
        if (!cache_ || stable_cache_key.empty()) {
            return {};
        }

        const std::string key{stable_cache_key};
        auto& entry = cache_->entries[key];
        if (!entry.track_identity_attempted && fs::exists(metadataCachePath(*cache_, key))) {
            entry = loadMetadataCache(*cache_, key);
        }
        const bool cached_identity_is_current = entry.identity.found && entry.track_identity_version >= kTrackIdentityVersion;
        const bool cached_identity_is_strong_enough = entry.identity.audio_fingerprint_verified || !acoustid_.available();
        if (cached_identity_is_current && cached_identity_is_strong_enough) {
            return entry.identity;
        }
        const bool should_retry_for_fingerprint =
            entry.identity.fingerprint.empty() && acoustid_.available() && !stream_uri.empty();
        if (entry.track_identity_attempted && entry.track_identity_version >= kTrackIdentityVersion && !should_retry_for_fingerprint) {
            return {};
        }
        if (!connectivity_ || !connectivity_->connected()) {
            logRuntime(RuntimeLogLevel::Warn, "identity", "Remote track identity skipped for " + pathUtf8String(lookup_path) + ": network disconnected");
            return {};
        }

        app::TrackIdentity identity{};
        if (acoustid_.available() && !stream_uri.empty()) {
            identity = acoustid_.lookupInput(stream_uri, pathUtf8String(lookup_path), seed_metadata);
        } else {
            logRuntime(RuntimeLogLevel::Debug, "identity", "AcoustID unavailable; falling back to MusicBrainz text lookup for remote " + pathUtf8String(lookup_path));
        }

        if (!identity.found && musicbrainz_.available()) {
            identity = identityFromMusicBrainz(musicbrainz_.lookup(lookup_path, *cache_, seed_metadata));
            if (identity.found) {
                logRuntime(RuntimeLogLevel::Info, "identity", "MusicBrainz identity hit for remote " + pathUtf8String(lookup_path));
            }
        }

        entry.track_identity_attempted = true;
        entry.track_identity_version = kTrackIdentityVersion;
        if (!identity.fingerprint.empty()) {
            entry.identity.fingerprint = identity.fingerprint;
            if (identity.metadata.duration_seconds) {
                entry.identity.metadata.duration_seconds = identity.metadata.duration_seconds;
            }
        }
        if (identity.found) {
            entry.identity = identity;
            entry.metadata = identity.metadata;
            if (!identity.release_mbid.empty()) entry.release_mbid = identity.release_mbid;
            if (!identity.release_group_mbid.empty()) entry.release_group_mbid = identity.release_group_mbid;
            logRuntime(RuntimeLogLevel::Info, "identity", "Remote track identity resolved by " + identity.source + " for " + pathUtf8String(lookup_path));
        } else {
            logRuntime(RuntimeLogLevel::Warn, "identity", "Remote track identity miss for " + pathUtf8String(lookup_path));
        }
        storeMetadataCache(*cache_, key, entry);
        return identity;
    }

private:
    std::shared_ptr<SharedRuntimeCache> cache_{};
    std::shared_ptr<app::ConnectivityProvider> connectivity_{};
    AcoustIdIdentityClient acoustid_{};
    MusicBrainzMetadataClient musicbrainz_{};
};

} // namespace

std::shared_ptr<app::TrackIdentityProvider> createHostTrackIdentityProvider(
    std::shared_ptr<runtime_detail::SharedRuntimeCache> cache,
    std::shared_ptr<app::ConnectivityProvider> connectivity)
{
    return std::make_shared<HostTrackIdentityProvider>(std::move(cache), std::move(connectivity));
}

} // namespace lofibox::platform::host
