// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "app/library_model.h"
#include "app/media_item.h"

namespace lofibox::library {

enum class LibraryChangeKind {
    Added,
    Modified,
    Removed,
};

struct LibraryFileChange {
    LibraryChangeKind kind{LibraryChangeKind::Added};
    std::filesystem::path path{};
    std::uintmax_t size_bytes{0};
    std::int64_t modified_time{0};
};

struct LibraryMigration {
    int from_version{0};
    int to_version{1};
    bool destructive{false};
};

struct SmartPlaylistRule {
    app::PlaylistKind kind{app::PlaylistKind::RecentlyAdded};
    int limit{50};
};

struct TrackIdentityKey {
    std::string recording_mbid{};
    std::string fingerprint{};
    std::string title{};
    std::string artist{};
    int duration_seconds{0};
};

struct SourceGroupedSearchResult {
    std::string source_label{};
    std::vector<app::MediaItem> items{};
    std::string explanation{};
};

class LibraryGovernanceService {
public:
    [[nodiscard]] std::vector<LibraryFileChange> incrementalChanges(const app::LibraryModel& before, const std::vector<std::filesystem::path>& current_files) const;
    [[nodiscard]] std::vector<LibraryMigration> migrationPlan(int current_version, int target_version) const;
    [[nodiscard]] std::vector<int> smartPlaylist(const app::LibraryModel& model, const SmartPlaylistRule& rule) const;
    [[nodiscard]] std::vector<SourceGroupedSearchResult> groupSearchResults(std::vector<app::MediaItem> local, std::vector<app::MediaItem> remote) const;
    [[nodiscard]] bool sameRecording(const TrackIdentityKey& left, const TrackIdentityKey& right) const noexcept;
};

} // namespace lofibox::library
