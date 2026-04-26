// SPDX-License-Identifier: GPL-3.0-or-later

#include "metadata/metadata_merge_policy.h"

#include <algorithm>
#include <cctype>

namespace lofibox::metadata {
namespace {

std::string normalized(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool isPlaceholder(std::string value)
{
    value = normalized(std::move(value));
    return value.empty() || value == "unknown" || value == "unknown artist" || value == "unknown album";
}

} // namespace

void MetadataMergePolicy::mergeIntoTrack(app::TrackRecord& track, const app::TrackMetadata& metadata) const
{
    if (shouldReplaceText(track.title, metadata.title)) track.title = *metadata.title;
    if (shouldReplaceText(track.artist, metadata.artist)) track.artist = *metadata.artist;
    if (shouldReplaceText(track.album, metadata.album)) track.album = *metadata.album;
    if (shouldReplaceText(track.genre, metadata.genre)) track.genre = *metadata.genre;
    if (shouldReplaceText(track.composer, metadata.composer)) track.composer = *metadata.composer;
    if (metadata.duration_seconds && *metadata.duration_seconds > 0) track.duration_seconds = *metadata.duration_seconds;
}

bool MetadataMergePolicy::shouldReplaceText(const std::string& current, const std::optional<std::string>& candidate)
{
    if (!candidate || candidate->empty()) {
        return false;
    }
    if (current == *candidate) {
        return true;
    }
    return isPlaceholder(current);
}

} // namespace lofibox::metadata
