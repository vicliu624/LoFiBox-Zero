// SPDX-License-Identifier: GPL-3.0-or-later

#include "metadata/metadata_governance.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace lofibox::metadata {

MetadataGovernanceService::MetadataGovernanceService(std::filesystem::path fingerprint_index_path)
    : fingerprint_index_path_(std::move(fingerprint_index_path))
{
    if (!fingerprint_index_path_.empty()) {
        (void)loadFingerprintIndex();
    }
}

bool MetadataGovernanceService::rememberFingerprint(FingerprintIndexEntry entry)
{
    if (entry.fingerprint.empty()) {
        return false;
    }
    auto found = std::find_if(fingerprints_.begin(), fingerprints_.end(), [&](const auto& existing) {
        return existing.fingerprint == entry.fingerprint;
    });
    if (found == fingerprints_.end()) {
        fingerprints_.push_back(std::move(entry));
    } else {
        *found = std::move(entry);
    }
    if (!fingerprint_index_path_.empty()) {
        (void)saveFingerprintIndex();
    }
    return true;
}

std::optional<FingerprintIndexEntry> MetadataGovernanceService::lookupFingerprint(std::string_view fingerprint) const
{
    const auto found = std::find_if(fingerprints_.begin(), fingerprints_.end(), [&](const auto& entry) {
        return entry.fingerprint == fingerprint;
    });
    if (found == fingerprints_.end()) {
        return std::nullopt;
    }
    return *found;
}

std::vector<MetadataConflict> MetadataGovernanceService::conflicts(const app::TrackMetadata& existing, const app::TrackMetadata& proposed, std::string source, double confidence) const
{
    std::vector<MetadataConflict> result{};
    const auto add = [&](std::string field, const std::optional<std::string>& current, const std::optional<std::string>& next) {
        if (current && next && *current != "UNKNOWN" && *current != *next) {
            result.push_back({std::move(field), *current, *next, source, confidence});
        }
    };
    add("title", existing.title, proposed.title);
    add("artist", existing.artist, proposed.artist);
    add("album", existing.album, proposed.album);
    add("genre", existing.genre, proposed.genre);
    add("composer", existing.composer, proposed.composer);
    return result;
}

ArtworkProvenance MetadataGovernanceService::chooseArtwork(
    std::optional<ArtworkProvenance> embedded,
    std::optional<ArtworkProvenance> directory,
    std::optional<ArtworkProvenance> online,
    std::optional<ArtworkProvenance> cache) const
{
    std::vector<ArtworkProvenance> candidates{};
    if (embedded) {
        embedded->priority = 100;
        embedded->explanation = "Embedded cover art wins because it travels with the file.";
        candidates.push_back(*embedded);
    }
    if (directory) {
        directory->priority = 80;
        directory->explanation = "Directory cover art is preferred when embedded art is missing.";
        candidates.push_back(*directory);
    }
    if (online) {
        online->priority = 60;
        online->explanation = "Online cover art is accepted after local sources.";
        candidates.push_back(*online);
    }
    if (cache) {
        cache->priority = 40;
        cache->explanation = "Cached cover art is a fallback for offline continuity.";
        candidates.push_back(*cache);
    }
    if (candidates.empty()) {
        return {ArtworkSourceKind::Placeholder, {}, "No trusted artwork source is available.", 0};
    }
    return *std::max_element(candidates.begin(), candidates.end(), [](const auto& left, const auto& right) {
        return left.priority < right.priority;
    });
}

TagFormat MetadataGovernanceService::detectTagFormat(const std::filesystem::path& path) const
{
    auto extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    if (extension == ".mp3" || extension == ".aac") {
        return TagFormat::Id3;
    }
    if (extension == ".flac") {
        return TagFormat::FlacVorbis;
    }
    if (extension == ".ogg" || extension == ".opus") {
        return TagFormat::OggVorbis;
    }
    if (extension == ".m4a" || extension == ".mp4" || extension == ".alac") {
        return TagFormat::Mp4;
    }
    return TagFormat::Unknown;
}

bool MetadataGovernanceService::writebackSupported(TagFormat format) const noexcept
{
    switch (format) {
    case TagFormat::Id3:
    case TagFormat::FlacVorbis:
    case TagFormat::OggVorbis:
    case TagFormat::Mp4:
        return true;
    case TagFormat::Unknown:
        return false;
    }
    return false;
}

bool MetadataGovernanceService::loadFingerprintIndex()
{
    fingerprints_.clear();
    if (fingerprint_index_path_.empty()) {
        return false;
    }
    std::ifstream input(fingerprint_index_path_, std::ios::binary);
    if (!input) {
        return true;
    }
    std::string line{};
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream fields(line);
        FingerprintIndexEntry entry{};
        std::string found{};
        if (std::getline(fields, entry.fingerprint, '\t')
            && std::getline(fields, found, '\t')
            && std::getline(fields, entry.identity.recording_mbid, '\t')) {
            entry.path = found;
            fingerprints_.push_back(std::move(entry));
        }
    }
    return true;
}

bool MetadataGovernanceService::saveFingerprintIndex() const
{
    if (fingerprint_index_path_.empty()) {
        return false;
    }
    std::error_code ec{};
    std::filesystem::create_directories(fingerprint_index_path_.parent_path(), ec);
    if (ec) {
        return false;
    }
    std::ofstream output(fingerprint_index_path_, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }
    output << "# LoFiBox fingerprint index v1\n";
    for (const auto& entry : fingerprints_) {
        output << entry.fingerprint << '\t' << entry.path.string() << '\t' << entry.identity.recording_mbid << '\n';
    }
    return static_cast<bool>(output);
}

} // namespace lofibox::metadata
