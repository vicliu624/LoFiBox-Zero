// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_provider_factories.h"

#include <filesystem>
#include <memory>
#include <string>

#include "platform/host/runtime_enrichment_clients.h"
#include "platform/host/runtime_logger.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host {
namespace {
using namespace runtime_detail;
class FfprobeMetadataProvider final : public app::MetadataProvider {
public:
    explicit FfprobeMetadataProvider(
        std::shared_ptr<SharedRuntimeCache> cache,
        std::shared_ptr<app::ConnectivityProvider> connectivity,
        std::shared_ptr<app::TagWriter> tag_writer,
        std::shared_ptr<app::TrackIdentityProvider> track_identity_provider)
        : cache_(std::move(cache))
        , connectivity_(std::move(connectivity))
        , tag_writer_(std::move(tag_writer))
        , track_identity_provider_(std::move(track_identity_provider))
    {
    }

    [[nodiscard]] bool available() const override
    {
        return embedded_reader_.available();
    }

    [[nodiscard]] std::string displayName() const override
    {
        return "BUILT-IN";
    }

    [[nodiscard]] app::TrackMetadata read(const fs::path& path, app::MetadataReadMode mode = app::MetadataReadMode::AllowOnline) const override
    {
        if (!cache_) {
            return {};
        }

        const auto key = cacheKeyForPath(path);
        auto& entry = cache_->entries[key];
        if (!entry.online_metadata_attempted && fs::exists(metadataCachePath(*cache_, key))) {
            entry = loadMetadataCache(*cache_, key);
            logRuntime(RuntimeLogLevel::Debug, "metadata", "Metadata cache loaded for " + pathUtf8String(path));
        }
        logRuntime(RuntimeLogLevel::Debug, "metadata", "Read metadata for " + pathUtf8String(path));

        const auto embedded = embedded_reader_.read(path);
        if (embedded.title) entry.metadata.title = embedded.title;
        if (embedded.artist) entry.metadata.artist = embedded.artist;
        if (embedded.album) entry.metadata.album = embedded.album;
        if (embedded.genre) entry.metadata.genre = embedded.genre;
        if (embedded.composer) entry.metadata.composer = embedded.composer;
        if (embedded.duration_seconds) entry.metadata.duration_seconds = embedded.duration_seconds;
        const bool embedded_conflicts_with_filename =
            hasCoreMetadata(entry.metadata) && !metadataCompatibleWithFilename(path, entry.metadata);
        if (embedded_conflicts_with_filename) {
            logRuntime(RuntimeLogLevel::Warn, "metadata", "Embedded metadata conflicts with filename for " + pathUtf8String(path));
            const auto filename_guesses = fallbackMetadataGuessesFromPath(path);
            if (!filename_guesses.empty()) {
                entry.metadata.title = filename_guesses.front().title;
                entry.metadata.artist = filename_guesses.front().artist;
                entry.metadata.album.reset();
            }
        } else if (hasCoreMetadata(entry.metadata)) {
            logRuntime(RuntimeLogLevel::Debug, "metadata", "Embedded metadata hit for " + pathUtf8String(path));
        }

        if (mode == app::MetadataReadMode::LocalOnly) {
            return entry.metadata;
        }

        const bool needs_core_metadata = !hasCoreMetadata(entry.metadata) || embedded_conflicts_with_filename;
        const bool needs_release_identifiers = entry.release_mbid.empty() && entry.release_group_mbid.empty();
        if ((needs_core_metadata || needs_release_identifiers)
            && !entry.online_metadata_attempted
            && ((track_identity_provider_ && track_identity_provider_->available()) || online_lookup_.available())
            && connectivity_
            && connectivity_->connected()) {
            logRuntime(RuntimeLogLevel::Info, "metadata", "Online metadata lookup started for " + pathUtf8String(path));
            MusicBrainzLookupResult online{};
            app::TrackIdentity identity{};
            if (track_identity_provider_ && track_identity_provider_->available()) {
                identity = track_identity_provider_->resolve(path, entry.metadata);
                if (identity.found) {
                    online.found = true;
                    online.metadata = identity.metadata;
                    online.recording_mbid = identity.recording_mbid;
                    online.release_mbid = identity.release_mbid;
                    online.release_group_mbid = identity.release_group_mbid;
                    online.confidence = identity.confidence;
                }
            }
            if (!online.found && online_lookup_.available()) {
                online = online_lookup_.lookup(path, *cache_, entry.metadata);
            }
            if (online.found) {
                if (!entry.metadata.title || embedded_conflicts_with_filename) entry.metadata.title = online.metadata.title;
                if (!entry.metadata.artist || embedded_conflicts_with_filename) entry.metadata.artist = online.metadata.artist;
                if (!entry.metadata.album || embedded_conflicts_with_filename) entry.metadata.album = online.metadata.album;
            }
            if (!online.release_mbid.empty()) entry.release_mbid = online.release_mbid;
            if (!online.release_group_mbid.empty()) entry.release_group_mbid = online.release_group_mbid;

            entry.online_metadata_attempted = true;
            storeMetadataCache(*cache_, key, entry);
            logRuntime(
                online.found ? RuntimeLogLevel::Info : RuntimeLogLevel::Warn,
                "metadata",
                std::string(online.found ? "Online metadata hit for " : "Online metadata miss for ") + pathUtf8String(path));
            if (online.found && tag_writer_ && tag_writer_->available()) {
                app::TagWriteRequest request{};
                request.metadata = entry.metadata;
                if (identity.found) {
                    request.identity = identity;
                }
                request.only_fill_missing = !embedded_conflicts_with_filename;
                if (tag_writer_->write(path, request)) {
                logRuntime(RuntimeLogLevel::Info, "metadata", "Wrote enriched metadata back to file: " + pathUtf8String(path));
            } else {
                    logRuntime(RuntimeLogLevel::Warn, "metadata", "Failed to write enriched metadata back to file: " + pathUtf8String(path));
                }
            } else if (!online.found && embedded_conflicts_with_filename && tag_writer_ && tag_writer_->available()) {
                app::TagWriteRequest request{};
                request.metadata = entry.metadata;
                request.only_fill_missing = false;
                request.clear_missing_metadata = true;
                if (tag_writer_->write(path, request)) {
                    logRuntime(RuntimeLogLevel::Info, "metadata", "Wrote filename-derived metadata back to conflicted file: " + pathUtf8String(path));
                } else {
                    logRuntime(RuntimeLogLevel::Warn, "metadata", "Failed to write filename-derived metadata back to conflicted file: " + pathUtf8String(path));
                }
            }
        } else if ((needs_core_metadata || needs_release_identifiers) && !entry.online_metadata_attempted) {
            std::string reason = "unknown";
            if (!online_lookup_.available()) {
                reason = "curl unavailable";
            } else if (!connectivity_) {
                reason = "connectivity provider missing";
            } else if (!connectivity_->connected()) {
                reason = "network disconnected";
            }
            logRuntime(RuntimeLogLevel::Warn, "metadata", "Online metadata skipped for " + pathUtf8String(path) + ": " + reason);
        }

        return entry.metadata;
    }

private:
    std::shared_ptr<SharedRuntimeCache> cache_{};
    std::shared_ptr<app::ConnectivityProvider> connectivity_{};
    std::shared_ptr<app::TagWriter> tag_writer_{};
    std::shared_ptr<app::TrackIdentityProvider> track_identity_provider_{};
    FfprobeMetadataReader embedded_reader_{};
    MusicBrainzMetadataClient online_lookup_{};
};

} // namespace

std::shared_ptr<app::MetadataProvider> createHostMetadataProvider(
    std::shared_ptr<runtime_detail::SharedRuntimeCache> cache,
    std::shared_ptr<app::ConnectivityProvider> connectivity,
    std::shared_ptr<app::TagWriter> tag_writer,
    std::shared_ptr<app::TrackIdentityProvider> track_identity_provider)
{
    return std::make_shared<FfprobeMetadataProvider>(
        std::move(cache),
        std::move(connectivity),
        std::move(tag_writer),
        std::move(track_identity_provider));
}

} // namespace lofibox::platform::host
