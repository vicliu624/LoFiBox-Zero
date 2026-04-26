// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/library_scanner.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#if !defined(_WIN32)
#include <dirent.h>
#include <sys/stat.h>
#endif

namespace fs = std::filesystem;

namespace lofibox::app {
namespace {

std::string upperText(std::string_view text)
{
    std::string result{};
    result.reserve(text.size());
    for (const unsigned char ch : text) {
        if (ch < 0x80U) {
            result.push_back(static_cast<char>(std::toupper(ch)));
        } else {
            result.push_back(static_cast<char>(ch));
        }
    }
    return result;
}

std::string displayOrUnknown(std::string_view text)
{
    if (text.empty()) {
        return std::string(kUnknownMetadata);
    }
    return std::string(text);
}

std::string preferMetadataOrFallback(const std::optional<std::string>& metadata_value, std::string fallback)
{
    if (metadata_value && !metadata_value->empty()) {
        return *metadata_value;
    }
    return displayOrUnknown(fallback);
}

std::string pathUtf8String(const fs::path& path)
{
#if defined(_WIN32)
    const auto u8 = path.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
#else
    return path.string();
#endif
}

std::int64_t toUnixSeconds(const fs::file_time_type& time)
{
    const auto system_now = std::chrono::system_clock::now();
    const auto file_now = fs::file_time_type::clock::now();
    const auto translated = std::chrono::time_point_cast<std::chrono::system_clock::duration>(time - file_now + system_now);
    return std::chrono::duration_cast<std::chrono::seconds>(translated.time_since_epoch()).count();
}

bool isAudioCandidate(const fs::path& path)
{
    if (!path.has_extension()) {
        return false;
    }

    std::string extension = upperText(path.extension().string());
    static const std::unordered_set<std::string> kSupported{
        ".MP3", ".WAV", ".AAC", ".FLAC", ".OGG", ".OPUS", ".ALAC", ".APE", ".AIFF"};
    return kSupported.contains(extension);
}

void collectRegularFiles(const fs::path& root, std::vector<fs::path>& files, bool& degraded)
{
#if defined(_WIN32)
    std::error_code iterator_error{};
    fs::recursive_directory_iterator iterator{
        root,
        fs::directory_options::skip_permission_denied,
        iterator_error};

    if (iterator_error) {
        degraded = true;
        return;
    }

    fs::recursive_directory_iterator end{};
    while (iterator != end) {
        const auto entry = *iterator;
        std::error_code status_error{};
        if (entry.is_regular_file(status_error) && !status_error) {
            files.push_back(entry.path());
        } else if (status_error) {
            degraded = true;
        }

        std::error_code increment_error{};
        iterator.increment(increment_error);
        if (increment_error) {
            degraded = true;
            increment_error.clear();
        }
    }
#else
    DIR* directory = opendir(root.c_str());
    if (directory == nullptr) {
        degraded = true;
        return;
    }

    while (dirent* entry = readdir(directory)) {
        const std::string_view name{entry->d_name};
        if (name == "." || name == "..") {
            continue;
        }

        const fs::path child = root / std::string{name};
        struct stat status {};
        if (lstat(child.c_str(), &status) != 0) {
            degraded = true;
            continue;
        }

        if (S_ISDIR(status.st_mode)) {
            collectRegularFiles(child, files, degraded);
        } else if (S_ISREG(status.st_mode)) {
            files.push_back(child);
        }
    }

    closedir(directory);
#endif
}

std::vector<fs::path> defaultRoots(const std::vector<fs::path>& requested_roots)
{
    if (!requested_roots.empty()) {
        return requested_roots;
    }

    std::vector<fs::path> roots{};
    std::string env_value{};
#if defined(_MSC_VER)
    char* buffer = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&buffer, &size, "LOFIBOX_MEDIA_ROOT") == 0 && buffer != nullptr) {
        env_value.assign(buffer);
        std::free(buffer);
    }
#else
    if (const char* env = std::getenv("LOFIBOX_MEDIA_ROOT")) {
        env_value = env;
    }
#endif

    if (!env_value.empty()) {
        std::string current{};
        for (const char ch : env_value) {
            if (ch == ';') {
                if (!current.empty()) {
                    roots.emplace_back(current);
                    current.clear();
                }
                continue;
            }
            current.push_back(ch);
        }
        if (!current.empty()) {
            roots.emplace_back(current);
        }
    }

    if (roots.empty()) {
        roots.emplace_back("/music");
    }

    return roots;
}

StorageInfo scanStorageInfo(const std::vector<fs::path>& roots)
{
    for (const auto& root : roots) {
        std::error_code ec{};
        if (!fs::exists(root, ec)) {
            continue;
        }

        const auto info = fs::space(root, ec);
        if (ec) {
            continue;
        }

        return StorageInfo{true, info.capacity, info.available};
    }

    return {};
}

} // namespace

LibraryModel scanLibrary(const std::vector<fs::path>& requested_roots, const MetadataProvider& metadata_provider)
{
    LibraryModel model{};
    const auto roots = defaultRoots(requested_roots);
    model.storage = scanStorageInfo(roots);

    int next_id = 1;
    std::int64_t fallback_added = 1;

    for (const auto& root : roots) {
        std::error_code exists_error{};
        if (!fs::exists(root, exists_error) || exists_error) {
            if (!exists_error) {
                model.degraded = true;
            }
            continue;
        }

        std::vector<fs::path> files{};
        collectRegularFiles(root, files, model.degraded);

        for (const auto& path : files) {
            if (!isAudioCandidate(path)) {
                continue;
            }

            TrackRecord track{};
            track.id = next_id++;
            track.path = path;
            const auto metadata = metadata_provider.read(path, MetadataReadMode::LocalOnly);
            track.title = preferMetadataOrFallback(metadata.title, pathUtf8String(path.stem()));
            track.album = preferMetadataOrFallback(metadata.album, pathUtf8String(path.parent_path().filename()));
            track.artist = preferMetadataOrFallback(metadata.artist, pathUtf8String(path.parent_path().parent_path().filename()));
            track.genre = preferMetadataOrFallback(metadata.genre, std::string(kUnknownMetadata));
            track.composer = preferMetadataOrFallback(metadata.composer, std::string(kUnknownMetadata));

            std::error_code write_error{};
            const auto last_write = fs::last_write_time(path, write_error);
            track.added_time = write_error ? fallback_added++ : toUnixSeconds(last_write);
            track.duration_seconds = metadata.duration_seconds.value_or(150 + static_cast<int>((std::hash<std::string>{}(path.string()) % 210)));

            model.tracks.push_back(std::move(track));
        }
    }

    std::sort(model.tracks.begin(), model.tracks.end(), [](const TrackRecord& lhs, const TrackRecord& rhs) {
        if (lhs.artist != rhs.artist) {
            return lhs.artist < rhs.artist;
        }
        if (lhs.album != rhs.album) {
            return lhs.album < rhs.album;
        }
        return lhs.title < rhs.title;
    });

    std::set<std::string> artist_set{};
    std::set<std::string> genre_set{};
    std::set<std::string> composer_set{};
    std::map<std::pair<std::string, std::string>, std::vector<int>> album_map{};
    std::unordered_map<std::string, std::set<std::string>> compilation_artists{};
    std::unordered_map<std::string, std::vector<int>> compilation_tracks{};

    for (const auto& track : model.tracks) {
        artist_set.insert(track.artist);
        genre_set.insert(track.genre);
        composer_set.insert(track.composer);
        album_map[{track.album, track.artist}].push_back(track.id);
        compilation_artists[track.album].insert(track.artist);
        compilation_tracks[track.album].push_back(track.id);
    }

    model.artists.assign(artist_set.begin(), artist_set.end());
    model.genres.assign(genre_set.begin(), genre_set.end());
    model.composers.assign(composer_set.begin(), composer_set.end());

    for (const auto& [key, ids] : album_map) {
        model.albums.push_back(AlbumRecord{key.first, key.second, ids});
    }
    std::sort(model.albums.begin(), model.albums.end(), [](const AlbumRecord& lhs, const AlbumRecord& rhs) {
        if (lhs.album != rhs.album) {
            return lhs.album < rhs.album;
        }
        return lhs.artist < rhs.artist;
    });

    for (const auto& [album, artists] : compilation_artists) {
        if (artists.size() <= 1) {
            continue;
        }
        model.compilations.push_back(CompilationRecord{album, compilation_tracks[album]});
    }
    std::sort(model.compilations.begin(), model.compilations.end(), [](const CompilationRecord& lhs, const CompilationRecord& rhs) {
        return lhs.album < rhs.album;
    });

    return model;
}

} // namespace lofibox::app
