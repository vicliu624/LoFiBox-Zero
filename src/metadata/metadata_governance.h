// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "app/runtime_services.h"

namespace lofibox::metadata {

struct FingerprintIndexEntry {
    std::string fingerprint{};
    std::filesystem::path path{};
    app::TrackIdentity identity{};
};

struct MetadataConflict {
    std::string field{};
    std::string existing_value{};
    std::string proposed_value{};
    std::string source{};
    double confidence{0.0};
};

enum class ArtworkSourceKind {
    Embedded,
    Directory,
    Online,
    Cache,
    Placeholder,
};

struct ArtworkProvenance {
    ArtworkSourceKind source{ArtworkSourceKind::Placeholder};
    std::filesystem::path path{};
    std::string explanation{};
    int priority{0};
};

enum class TagFormat {
    Id3,
    FlacVorbis,
    OggVorbis,
    Mp4,
    Unknown,
};

class MetadataGovernanceService {
public:
    explicit MetadataGovernanceService(std::filesystem::path fingerprint_index_path = {});

    bool rememberFingerprint(FingerprintIndexEntry entry);
    [[nodiscard]] std::optional<FingerprintIndexEntry> lookupFingerprint(std::string_view fingerprint) const;
    [[nodiscard]] std::vector<MetadataConflict> conflicts(const app::TrackMetadata& existing, const app::TrackMetadata& proposed, std::string source, double confidence) const;
    [[nodiscard]] ArtworkProvenance chooseArtwork(std::optional<ArtworkProvenance> embedded, std::optional<ArtworkProvenance> directory, std::optional<ArtworkProvenance> online, std::optional<ArtworkProvenance> cache) const;
    [[nodiscard]] TagFormat detectTagFormat(const std::filesystem::path& path) const;
    [[nodiscard]] bool writebackSupported(TagFormat format) const noexcept;
    [[nodiscard]] bool loadFingerprintIndex();
    [[nodiscard]] bool saveFingerprintIndex() const;

private:
    std::filesystem::path fingerprint_index_path_{};
    std::vector<FingerprintIndexEntry> fingerprints_{};
};

} // namespace lofibox::metadata
