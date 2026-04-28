// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/media_search_service.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace lofibox::app {
namespace {

std::string lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return ch < 0x80U ? static_cast<char>(std::tolower(ch)) : static_cast<char>(ch);
    });
    return value;
}

bool matches(const TrackRecord& track, const std::string& query)
{
    if (query.empty()) {
        return true;
    }
    return lower(track.title).find(query) != std::string::npos
        || lower(track.artist).find(query) != std::string::npos
        || lower(track.album).find(query) != std::string::npos;
}

} // namespace

MediaSearchResult MediaSearchService::searchLocal(const LibraryModel& library, std::string_view query, int limit)
{
    MediaSearchResult result{};
    const auto normalized_query = lower(std::string(query));
    for (const auto& track : library.tracks) {
        if (limit >= 0 && static_cast<int>(result.local_items.size()) >= limit) {
            break;
        }
        if (matches(track, normalized_query)) {
            result.local_items.push_back(mediaItemFromTrackRecord(track));
        }
    }
    return result;
}

std::vector<MediaItem> MediaSearchService::mapRemoteResults(const RemoteServerProfile& profile, const std::vector<RemoteTrack>& tracks)
{
    std::vector<MediaItem> items{};
    items.reserve(tracks.size());
    for (const auto& track : tracks) {
        items.push_back(mediaItemFromRemoteTrack(profile, track));
    }
    return items;
}

} // namespace lofibox::app
