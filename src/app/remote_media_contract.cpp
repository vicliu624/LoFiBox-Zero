// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/remote_media_contract.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>

#include "app/remote_profile_store.h"
#include "metadata/match_confidence_guard.h"

namespace lofibox::app {
namespace {

std::string encodeValue(std::string_view value)
{
    std::string encoded{};
    for (const char ch : value) {
        if (ch == '\\' || ch == '\n' || ch == '=') {
            encoded.push_back('\\');
            if (ch == '\n') {
                encoded.push_back('n');
            } else {
                encoded.push_back(ch);
            }
        } else {
            encoded.push_back(ch);
        }
    }
    return encoded;
}

std::string decodeValue(std::string_view value)
{
    std::string decoded{};
    bool escaping = false;
    for (const char ch : value) {
        if (escaping) {
            decoded.push_back(ch == 'n' ? '\n' : ch);
            escaping = false;
        } else if (ch == '\\') {
            escaping = true;
        } else {
            decoded.push_back(ch);
        }
    }
    if (escaping) {
        decoded.push_back('\\');
    }
    return decoded;
}

bool missingText(const std::string& value, const std::string& fallback_id)
{
    if (value.empty()) {
        return true;
    }
    auto normalized = value;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    if (normalized == "unknown"
        || normalized == "unknown artist"
        || normalized == "unknown album"
        || normalized == "remote stream") {
        return true;
    }
    return !fallback_id.empty() && value == fallback_id;
}

bool looksLikeMojibake(std::string_view value) noexcept
{
    constexpr std::string_view kReplacement{"\xEF\xBF\xBD"};
    constexpr std::string_view kUtf8AsLatin1A{"\xC3\x83"};
    constexpr std::string_view kUtf8AsLatin1B{"\xC3\x82"};
    constexpr std::string_view kGbkLatin1TitleMarker{"\xC3\x87"};
    constexpr std::string_view kGbkLatin1ArtistMarker{"\xC3\x8E"};
    return value.find(kReplacement) != std::string_view::npos
        || value.find(kUtf8AsLatin1A) != std::string_view::npos
        || value.find(kUtf8AsLatin1B) != std::string_view::npos
        || value.find(kGbkLatin1TitleMarker) != std::string_view::npos
        || value.find(kGbkLatin1ArtistMarker) != std::string_view::npos;
}

bool governedTextNeedsReplacement(const std::string& value, const std::string& fallback_id) noexcept
{
    return missingText(value, fallback_id) || looksLikeMojibake(value);
}

bool shouldAcceptGovernedText(
    const std::string& current,
    const std::optional<std::string>& candidate,
    const std::string& fallback_id) noexcept
{
    return candidate
        && !candidate->empty()
        && (current == *candidate || governedTextNeedsReplacement(current, fallback_id));
}

bool shouldAcceptGovernedText(
    const std::string& current,
    const std::optional<std::string>& candidate,
    const std::string& fallback_id,
    bool authoritative_identity) noexcept
{
    return candidate
        && !candidate->empty()
        && (authoritative_identity
            || current == *candidate
            || governedTextNeedsReplacement(current, fallback_id));
}

std::string pathSafeToken(std::string value)
{
    for (char& ch : value) {
        switch (ch) {
        case '/':
        case '\\':
        case ':':
        case '*':
        case '?':
        case '"':
        case '<':
        case '>':
        case '|':
            ch = ' ';
            break;
        default:
            break;
        }
    }
    std::string compact{};
    compact.reserve(value.size());
    bool previous_space = false;
    for (const char ch : value) {
        if (ch == ' ') {
            if (!previous_space) {
                compact.push_back(ch);
            }
            previous_space = true;
        } else {
            compact.push_back(ch);
            previous_space = false;
        }
    }
    return compact.empty() ? std::string{"remote-track"} : compact;
}

void emit(std::ostringstream& output, std::string_view key, std::string_view value)
{
    output << key << '=' << encodeValue(value) << '\n';
}

int parseInt(std::string_view value)
{
    try {
        return value.empty() ? 0 : std::stoi(std::string{value});
    } catch (...) {
        return 0;
    }
}

std::string kindName(RemoteCatalogNodeKind kind)
{
    switch (kind) {
    case RemoteCatalogNodeKind::Artists: return "artists";
    case RemoteCatalogNodeKind::Artist: return "artist";
    case RemoteCatalogNodeKind::Albums: return "albums";
    case RemoteCatalogNodeKind::Album: return "album";
    case RemoteCatalogNodeKind::Tracks: return "tracks";
    case RemoteCatalogNodeKind::Genres: return "genres";
    case RemoteCatalogNodeKind::Genre: return "genre";
    case RemoteCatalogNodeKind::Playlists: return "playlists";
    case RemoteCatalogNodeKind::Playlist: return "playlist";
    case RemoteCatalogNodeKind::Folders: return "folders";
    case RemoteCatalogNodeKind::Folder: return "folder";
    case RemoteCatalogNodeKind::Favorites: return "favorites";
    case RemoteCatalogNodeKind::RecentlyAdded: return "recently-added";
    case RemoteCatalogNodeKind::RecentlyPlayed: return "recently-played";
    case RemoteCatalogNodeKind::Stations: return "stations";
    case RemoteCatalogNodeKind::Root: return "root";
    }
    return "root";
}

RemoteCatalogNodeKind parseKind(std::string_view value)
{
    if (value == "artists") return RemoteCatalogNodeKind::Artists;
    if (value == "artist") return RemoteCatalogNodeKind::Artist;
    if (value == "albums") return RemoteCatalogNodeKind::Albums;
    if (value == "album") return RemoteCatalogNodeKind::Album;
    if (value == "tracks") return RemoteCatalogNodeKind::Tracks;
    if (value == "genres") return RemoteCatalogNodeKind::Genres;
    if (value == "genre") return RemoteCatalogNodeKind::Genre;
    if (value == "playlists") return RemoteCatalogNodeKind::Playlists;
    if (value == "playlist") return RemoteCatalogNodeKind::Playlist;
    if (value == "folders") return RemoteCatalogNodeKind::Folders;
    if (value == "folder") return RemoteCatalogNodeKind::Folder;
    if (value == "favorites") return RemoteCatalogNodeKind::Favorites;
    if (value == "recently-added") return RemoteCatalogNodeKind::RecentlyAdded;
    if (value == "recently-played") return RemoteCatalogNodeKind::RecentlyPlayed;
    if (value == "stations") return RemoteCatalogNodeKind::Stations;
    return RemoteCatalogNodeKind::Root;
}

} // namespace

std::string remoteMediaCacheKey(const RemoteServerProfile& profile, std::string_view item_id)
{
    return "remote-media-" + remoteServerKindToString(profile.kind) + "-" + profile.id + "-" + std::string(item_id);
}

std::string remoteDirectoryCacheKey(const RemoteServerProfile& profile, const RemoteCatalogNode& parent)
{
    const std::string id = parent.id.empty() ? "root" : parent.id;
    return remoteServerKindToString(profile.kind) + "-" + profile.id + "-" + kindName(parent.kind) + "-" + id;
}

std::string serializeRemoteTrackCache(const RemoteTrack& track)
{
    std::ostringstream output{};
    output << "version=2\n";
    emit(output, "id", track.id);
    emit(output, "title", track.title);
    emit(output, "artist", track.artist);
    emit(output, "album", track.album);
    emit(output, "album_id", track.album_id);
    output << "duration=" << track.duration_seconds << '\n';
    emit(output, "source_id", track.source_id);
    emit(output, "source_label", track.source_label);
    emit(output, "artwork_key", track.artwork_key);
    emit(output, "artwork_url", track.artwork_url);
    emit(output, "lyrics_plain", track.lyrics_plain);
    emit(output, "lyrics_synced", track.lyrics_synced);
    emit(output, "lyrics_source", track.lyrics_source);
    emit(output, "fingerprint", track.fingerprint);
    return output.str();
}

RemoteTrack parseRemoteTrackCache(std::string_view text)
{
    RemoteTrack track{};
    std::istringstream input{std::string{text}};
    std::string line{};
    while (std::getline(input, line)) {
        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        const auto key = line.substr(0, pos);
        const auto value = decodeValue(line.substr(pos + 1));
        if (key == "id") track.id = value;
        else if (key == "title") track.title = value;
        else if (key == "artist") track.artist = value;
        else if (key == "album") track.album = value;
        else if (key == "album_id") track.album_id = value;
        else if (key == "duration") track.duration_seconds = parseInt(value);
        else if (key == "source_id") track.source_id = value;
        else if (key == "source_label") track.source_label = value;
        else if (key == "artwork_key") track.artwork_key = value;
        else if (key == "artwork_url") track.artwork_url = value;
        else if (key == "lyrics_plain") track.lyrics_plain = value;
        else if (key == "lyrics_synced") track.lyrics_synced = value;
        else if (key == "lyrics_source") track.lyrics_source = value;
        else if (key == "fingerprint") track.fingerprint = value;
    }
    if (looksLikeMojibake(track.title)) track.title.clear();
    if (looksLikeMojibake(track.artist)) track.artist.clear();
    if (looksLikeMojibake(track.album)) track.album.clear();
    if (looksLikeMojibake(track.lyrics_plain)) track.lyrics_plain.clear();
    if (looksLikeMojibake(track.lyrics_synced)) track.lyrics_synced.clear();
    return track;
}

RemoteTrack mergeRemoteTrackCache(RemoteTrack current, const RemoteTrack& cached)
{
    if (!cached.title.empty() && missingText(current.title, current.id)) current.title = cached.title;
    if (!cached.artist.empty() && missingText(current.artist, current.id)) current.artist = cached.artist;
    if (!cached.album.empty() && missingText(current.album, current.id)) current.album = cached.album;
    if (!cached.album_id.empty() && current.album_id.empty()) current.album_id = cached.album_id;
    if (cached.duration_seconds > 0 && current.duration_seconds <= 0) current.duration_seconds = cached.duration_seconds;
    if (!cached.source_id.empty() && current.source_id.empty()) current.source_id = cached.source_id;
    if (!cached.source_label.empty() && current.source_label.empty()) current.source_label = cached.source_label;
    if (!cached.artwork_key.empty() && current.artwork_key.empty()) current.artwork_key = cached.artwork_key;
    if (!cached.artwork_url.empty() && current.artwork_url.empty()) current.artwork_url = cached.artwork_url;
    if (!cached.lyrics_plain.empty() && current.lyrics_plain.empty()) current.lyrics_plain = cached.lyrics_plain;
    if (!cached.lyrics_synced.empty() && current.lyrics_synced.empty()) current.lyrics_synced = cached.lyrics_synced;
    if (!cached.lyrics_source.empty() && current.lyrics_source.empty()) current.lyrics_source = cached.lyrics_source;
    if (!cached.fingerprint.empty() && current.fingerprint.empty()) current.fingerprint = cached.fingerprint;
    return current;
}

RemoteTrack remoteTrackFromCatalogNode(const RemoteCatalogNode& node)
{
    RemoteTrack track{};
    track.id = node.id;
    track.title = node.title;
    track.artist = node.artist;
    track.album = node.album;
    track.album_id = node.album_id;
    track.duration_seconds = node.duration_seconds;
    track.source_id = node.source_id;
    track.source_label = node.source_label;
    track.artwork_key = node.artwork_key;
    track.artwork_url = node.artwork_url;
    track.lyrics_plain = node.lyrics_plain;
    track.lyrics_synced = node.lyrics_synced;
    track.lyrics_source = node.lyrics_source;
    track.fingerprint = node.fingerprint;
    return track;
}

bool remoteTrackHasLocalCacheableFacts(const RemoteTrack& track) noexcept
{
    return !track.title.empty()
        || !track.artist.empty()
        || !track.album.empty()
        || !track.album_id.empty()
        || track.duration_seconds > 0
        || !track.artwork_key.empty()
        || !track.artwork_url.empty()
        || !track.lyrics_plain.empty()
        || !track.lyrics_synced.empty()
        || !track.fingerprint.empty();
}

bool remoteTrackNeedsLocalMetadataGovernance(const RemoteTrack& track) noexcept
{
    const bool metadata_needs_governance =
        governedTextNeedsReplacement(track.title, track.id)
        || governedTextNeedsReplacement(track.artist, {})
        || looksLikeMojibake(track.album);
    const bool lyrics_missing = track.lyrics_plain.empty() && track.lyrics_synced.empty();
    return metadata_needs_governance || lyrics_missing || track.fingerprint.empty();
}

TrackMetadata remoteTrackMetadataSeed(const RemoteTrack& track)
{
    TrackMetadata metadata{};
    if (!governedTextNeedsReplacement(track.title, track.id)) {
        metadata.title = track.title;
    }
    if (!governedTextNeedsReplacement(track.artist, {})) {
        metadata.artist = track.artist;
    }
    if (!governedTextNeedsReplacement(track.album, {})) {
        metadata.album = track.album;
    }
    if (track.duration_seconds > 0) {
        metadata.duration_seconds = track.duration_seconds;
    }
    return metadata;
}

std::filesystem::path remoteTrackSyntheticLookupPath(const RemoteServerProfile& profile, const RemoteTrack& track)
{
    const auto metadata = remoteTrackMetadataSeed(track);
    const std::string title = metadata.title.value_or(track.id.empty() ? std::string{"remote-track"} : track.id);
    const std::string artist = metadata.artist.value_or("");
    const std::string filename = artist.empty()
        ? pathSafeToken(title) + ".mp3"
        : pathSafeToken(artist + " - " + title) + ".mp3";
    return std::filesystem::path{"remote"} / pathSafeToken(profile.id.empty() ? remoteServerKindToString(profile.kind) : profile.id) / filename;
}

RemoteTrack mergeRemoteGovernedFacts(RemoteTrack current, const TrackMetadata& metadata, const TrackLyrics& lyrics)
{
    if (shouldAcceptGovernedText(current.title, metadata.title, current.id)) current.title = *metadata.title;
    if (shouldAcceptGovernedText(current.artist, metadata.artist, {})) current.artist = *metadata.artist;
    if (shouldAcceptGovernedText(current.album, metadata.album, {})) current.album = *metadata.album;
    if (metadata.duration_seconds && *metadata.duration_seconds > 0 && current.duration_seconds <= 0) {
        current.duration_seconds = *metadata.duration_seconds;
    }
    if (lyrics.plain && !lyrics.plain->empty()) current.lyrics_plain = *lyrics.plain;
    if (lyrics.synced && !lyrics.synced->empty()) current.lyrics_synced = *lyrics.synced;
    if (!lyrics.source.empty()) current.lyrics_source = lyrics.source;
    return current;
}

RemoteTrack mergeRemoteGovernedFacts(
    RemoteTrack current,
    const TrackMetadata& metadata,
    const TrackLyrics& lyrics,
    const TrackIdentity& identity)
{
    const bool authoritative_identity = ::lofibox::metadata::MatchConfidenceGuard{}.acceptsIdentity(identity);
    if (shouldAcceptGovernedText(current.title, metadata.title, current.id, authoritative_identity)) current.title = *metadata.title;
    if (shouldAcceptGovernedText(current.artist, metadata.artist, {}, authoritative_identity)) current.artist = *metadata.artist;
    if (shouldAcceptGovernedText(current.album, metadata.album, {}, authoritative_identity)) current.album = *metadata.album;
    if (metadata.duration_seconds
        && *metadata.duration_seconds > 0
        && (current.duration_seconds <= 0 || authoritative_identity)) {
        current.duration_seconds = *metadata.duration_seconds;
    }
    if (lyrics.plain && !lyrics.plain->empty()) current.lyrics_plain = *lyrics.plain;
    if (lyrics.synced && !lyrics.synced->empty()) current.lyrics_synced = *lyrics.synced;
    if (!lyrics.source.empty()) current.lyrics_source = lyrics.source;
    return current;
}

std::string remoteBrowseCachePayload(const std::vector<RemoteCatalogNode>& nodes)
{
    std::ostringstream output{};
    output << "version=1\n";
    for (const auto& node : nodes) {
        output << "node\n";
        emit(output, "kind", kindName(node.kind));
        emit(output, "id", node.id);
        emit(output, "title", node.title);
        emit(output, "subtitle", node.subtitle);
        emit(output, "artist", node.artist);
        emit(output, "album", node.album);
        emit(output, "album_id", node.album_id);
        output << "duration=" << node.duration_seconds << '\n';
        output << "playable=" << (node.playable ? "1" : "0") << '\n';
        output << "browsable=" << (node.browsable ? "1" : "0") << '\n';
        emit(output, "source_id", node.source_id);
        emit(output, "source_label", node.source_label);
        emit(output, "artwork_key", node.artwork_key);
        emit(output, "artwork_url", node.artwork_url);
        emit(output, "lyrics_plain", node.lyrics_plain);
        emit(output, "lyrics_synced", node.lyrics_synced);
        emit(output, "lyrics_source", node.lyrics_source);
        emit(output, "fingerprint", node.fingerprint);
    }
    return output.str();
}

std::vector<RemoteCatalogNode> parseRemoteBrowseCachePayload(std::string_view text)
{
    std::vector<RemoteCatalogNode> nodes{};
    RemoteCatalogNode current{};
    bool have_node = false;
    auto flush = [&]() {
        if (have_node) {
            nodes.push_back(current);
            current = {};
            have_node = false;
        }
    };

    std::istringstream input{std::string{text}};
    std::string line{};
    while (std::getline(input, line)) {
        if (line == "node") {
            flush();
            have_node = true;
            continue;
        }
        if (!have_node) {
            continue;
        }
        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        const auto key = line.substr(0, pos);
        const auto value = decodeValue(line.substr(pos + 1));
        if (key == "kind") current.kind = parseKind(value);
        else if (key == "id") current.id = value;
        else if (key == "title") current.title = value;
        else if (key == "subtitle") current.subtitle = value;
        else if (key == "artist") current.artist = value;
        else if (key == "album") current.album = value;
        else if (key == "album_id") current.album_id = value;
        else if (key == "duration") current.duration_seconds = parseInt(value);
        else if (key == "playable") current.playable = value == "1";
        else if (key == "browsable") current.browsable = value != "0";
        else if (key == "source_id") current.source_id = value;
        else if (key == "source_label") current.source_label = value;
        else if (key == "artwork_key") current.artwork_key = value;
        else if (key == "artwork_url") current.artwork_url = value;
        else if (key == "lyrics_plain") current.lyrics_plain = value;
        else if (key == "lyrics_synced") current.lyrics_synced = value;
        else if (key == "lyrics_source") current.lyrics_source = value;
        else if (key == "fingerprint") current.fingerprint = value;
    }
    flush();
    return nodes;
}

} // namespace lofibox::app
