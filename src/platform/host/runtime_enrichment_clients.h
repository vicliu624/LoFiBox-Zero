// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "app/runtime_services.h"
#include "core/canvas.h"
#include "platform/host/runtime_host_internal.h"

namespace lofibox::platform::host::runtime_detail {
namespace fs = std::filesystem;

struct MusicBrainzLookupResult {
    app::TrackMetadata metadata{};
    std::string recording_mbid{};
    std::string release_mbid{};
    std::string release_group_mbid{};
    double confidence{0.0};
    bool found{false};
};

struct AudioFingerprintResult {
    std::string fingerprint{};
    int duration_seconds{0};
    bool found{false};
};

class FfprobeMetadataReader {
public:
    FfprobeMetadataReader();
    [[nodiscard]] bool available() const;
    [[nodiscard]] app::TrackMetadata read(const fs::path& path) const;

private:
    std::optional<fs::path> executable_path_{};
};

class MusicBrainzMetadataClient {
public:
    MusicBrainzMetadataClient();
    [[nodiscard]] bool available() const;
    [[nodiscard]] MusicBrainzLookupResult lookup(
        const fs::path& path,
        SharedRuntimeCache& cache,
        const app::TrackMetadata& seed_metadata = {}) const;

private:
    std::optional<fs::path> curl_path_{};
};

class ChromaprintFingerprintProvider {
public:
    ChromaprintFingerprintProvider();
    [[nodiscard]] bool available() const;
    [[nodiscard]] AudioFingerprintResult fingerprint(const fs::path& path) const;
    [[nodiscard]] AudioFingerprintResult fingerprintInput(std::string_view input) const;

private:
    std::optional<fs::path> executable_path_{};
};

class AcoustIdIdentityClient {
public:
    AcoustIdIdentityClient();
    [[nodiscard]] bool available() const;
    [[nodiscard]] app::TrackIdentity lookup(const fs::path& path, const app::TrackMetadata& seed_metadata = {}) const;
    [[nodiscard]] app::TrackIdentity lookupInput(
        std::string_view input,
        std::string_view log_label,
        const app::TrackMetadata& seed_metadata = {}) const;

private:
    std::optional<fs::path> curl_path_{};
    std::string api_key_{};
    ChromaprintFingerprintProvider fingerprint_provider_{};
};

class FfmpegArtworkExtractor {
public:
    FfmpegArtworkExtractor();
    [[nodiscard]] bool available() const;
    [[nodiscard]] std::optional<core::Canvas> extractEmbedded(const fs::path& path, const fs::path& cache_path) const;
    [[nodiscard]] std::optional<core::Canvas> materialize(const fs::path& candidate, const fs::path& cache_path) const;

private:
    std::optional<fs::path> executable_path_{};
};

class CoverArtArchiveClient {
public:
    CoverArtArchiveClient();
    [[nodiscard]] bool available() const;
    [[nodiscard]] bool fetchFrontCover(std::string_view release_mbid, const fs::path& output_path) const;
    [[nodiscard]] bool fetchReleaseGroupFrontCover(std::string_view release_group_mbid, const fs::path& output_path) const;

private:
    std::optional<fs::path> curl_path_{};
    FfmpegArtworkExtractor extractor_{};
};

class LrclibClient {
public:
    LrclibClient();
    [[nodiscard]] bool available() const;
    [[nodiscard]] app::TrackLyrics fetch(const fs::path& path, const app::TrackMetadata& metadata) const;

private:
    std::optional<fs::path> curl_path_{};
};

class LyricsOvhClient {
public:
    LyricsOvhClient();
    [[nodiscard]] bool available() const;
    [[nodiscard]] app::TrackLyrics fetch(const fs::path& path, const app::TrackMetadata& metadata) const;

private:
    std::optional<fs::path> curl_path_{};
};

} // namespace lofibox::platform::host::runtime_detail
