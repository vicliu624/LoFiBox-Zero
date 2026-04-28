// SPDX-License-Identifier: GPL-3.0-or-later

#include "library/library_governance.h"

#include <algorithm>
#include <cstdlib>
#include <unordered_set>

namespace lofibox::library {

std::vector<LibraryFileChange> LibraryGovernanceService::incrementalChanges(const app::LibraryModel& before, const std::vector<std::filesystem::path>& current_files) const
{
    std::unordered_set<std::string> known{};
    for (const auto& track : before.tracks) {
        if (track.remote) {
            continue;
        }
        known.insert(track.path.lexically_normal().string());
    }
    std::unordered_set<std::string> current{};
    std::vector<LibraryFileChange> changes{};
    for (const auto& path : current_files) {
        const auto normalized = path.lexically_normal().string();
        current.insert(normalized);
        if (!known.contains(normalized)) {
            changes.push_back({LibraryChangeKind::Added, path, 0, 0});
        }
    }
    for (const auto& track : before.tracks) {
        if (track.remote) {
            continue;
        }
        const auto normalized = track.path.lexically_normal().string();
        if (!current.contains(normalized)) {
            changes.push_back({LibraryChangeKind::Removed, track.path, 0, 0});
        }
    }
    return changes;
}

std::vector<LibraryMigration> LibraryGovernanceService::migrationPlan(int current_version, int target_version) const
{
    std::vector<LibraryMigration> plan{};
    for (int version = current_version; version < target_version; ++version) {
        plan.push_back({version, version + 1, false});
    }
    return plan;
}

std::vector<int> LibraryGovernanceService::smartPlaylist(const app::LibraryModel& model, const SmartPlaylistRule& rule) const
{
    std::vector<app::TrackRecord> tracks = model.tracks;
    if (rule.kind == app::PlaylistKind::MostPlayed) {
        std::sort(tracks.begin(), tracks.end(), [](const auto& left, const auto& right) {
            return left.play_count > right.play_count;
        });
    } else if (rule.kind == app::PlaylistKind::RecentlyPlayed) {
        std::sort(tracks.begin(), tracks.end(), [](const auto& left, const auto& right) {
            return left.last_played > right.last_played;
        });
    } else {
        std::sort(tracks.begin(), tracks.end(), [](const auto& left, const auto& right) {
            return left.added_time > right.added_time;
        });
    }
    std::vector<int> result{};
    for (const auto& track : tracks) {
        if (static_cast<int>(result.size()) >= rule.limit) {
            break;
        }
        result.push_back(track.id);
    }
    return result;
}

std::vector<SourceGroupedSearchResult> LibraryGovernanceService::groupSearchResults(std::vector<app::MediaItem> local, std::vector<app::MediaItem> remote) const
{
    return {
        {"LOCAL LIBRARY", std::move(local), "Results from indexed local files."},
        {"REMOTE SOURCES", std::move(remote), "Results from connected remote media providers."},
    };
}

bool LibraryGovernanceService::sameRecording(const TrackIdentityKey& left, const TrackIdentityKey& right) const noexcept
{
    if (!left.recording_mbid.empty() && left.recording_mbid == right.recording_mbid) {
        return true;
    }
    if (!left.fingerprint.empty() && left.fingerprint == right.fingerprint) {
        return true;
    }
    return left.duration_seconds > 0
        && std::abs(left.duration_seconds - right.duration_seconds) <= 2
        && !left.title.empty()
        && left.title == right.title
        && left.artist == right.artist;
}

} // namespace lofibox::library
