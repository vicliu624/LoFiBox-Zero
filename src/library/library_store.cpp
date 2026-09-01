// SPDX-License-Identifier: GPL-3.0-or-later

#include "library/library_store.h"

#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace lofibox::library {
namespace {

constexpr int kStoreSchemaVersion = 2;

std::string pathUtf8String(const fs::path& path)
{
#if defined(_WIN32)
    const auto u8 = path.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
#else
    return path.string();
#endif
}

std::string hexEncode(std::string_view value)
{
    static constexpr char kDigits[] = "0123456789abcdef";
    std::string result{};
    result.reserve(value.size() * 2U);
    for (const unsigned char byte : value) {
        result.push_back(kDigits[(byte >> 4U) & 0x0FU]);
        result.push_back(kDigits[byte & 0x0FU]);
    }
    return result;
}

std::optional<unsigned char> hexNibble(char value)
{
    if (value >= '0' && value <= '9') return static_cast<unsigned char>(value - '0');
    if (value >= 'a' && value <= 'f') return static_cast<unsigned char>(value - 'a' + 10);
    if (value >= 'A' && value <= 'F') return static_cast<unsigned char>(value - 'A' + 10);
    return std::nullopt;
}

std::optional<std::string> hexDecode(std::string_view value)
{
    if (value.size() % 2U != 0U) {
        return std::nullopt;
    }
    std::string result{};
    result.reserve(value.size() / 2U);
    for (std::size_t index = 0; index < value.size(); index += 2U) {
        const auto high = hexNibble(value[index]);
        const auto low = hexNibble(value[index + 1U]);
        if (!high || !low) {
            return std::nullopt;
        }
        result.push_back(static_cast<char>((*high << 4U) | *low));
    }
    return result;
}

std::vector<std::string_view> splitTabs(std::string_view line)
{
    std::vector<std::string_view> fields{};
    std::size_t start = 0U;
    while (start <= line.size()) {
        const auto separator = line.find('\t', start);
        fields.push_back(line.substr(start, separator == std::string_view::npos ? std::string_view::npos : separator - start));
        if (separator == std::string_view::npos) {
            break;
        }
        start = separator + 1U;
    }
    return fields;
}

template <typename Integer>
std::optional<Integer> parseInteger(std::string_view text)
{
    Integer value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

std::optional<int> readSchemaVersion(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return std::nullopt;
    }
    std::string line{};
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        constexpr std::string_view kPrefix{"# LoFiBox library store schema="};
        if (!line.starts_with(kPrefix)) {
            return std::nullopt;
        }
        return parseInteger<int>(std::string_view{line}.substr(kPrefix.size()));
    }
    return std::nullopt;
}

void appendTrackField(std::ostream& output, std::string_view value)
{
    output << '\t' << hexEncode(value);
}

bool appendTrack(std::ostream& output, const app::TrackRecord& track)
{
    if (track.remote) {
        return true;
    }
    output << track.id;
    appendTrackField(output, pathUtf8String(track.path));
    appendTrackField(output, track.title);
    appendTrackField(output, track.artist);
    appendTrackField(output, track.album);
    appendTrackField(output, track.genre);
    appendTrackField(output, track.composer);
    output << '\t' << track.added_time
           << '\t' << track.duration_seconds
           << '\t' << track.play_count
           << '\t' << track.last_played
           << '\t' << track.file_size_bytes
           << '\t' << track.file_mtime_ticks;
    appendTrackField(output, track.fingerprint);
    output << '\n';
    return static_cast<bool>(output);
}

std::optional<app::TrackRecord> parseTrack(std::string_view line)
{
    const auto fields = splitTabs(line);
    constexpr std::size_t kFieldCount = 14U;
    if (fields.size() != kFieldCount) {
        return std::nullopt;
    }

    const auto id = parseInteger<int>(fields[0]);
    const auto path = hexDecode(fields[1]);
    const auto title = hexDecode(fields[2]);
    const auto artist = hexDecode(fields[3]);
    const auto album = hexDecode(fields[4]);
    const auto genre = hexDecode(fields[5]);
    const auto composer = hexDecode(fields[6]);
    const auto added_time = parseInteger<std::int64_t>(fields[7]);
    const auto duration = parseInteger<int>(fields[8]);
    const auto play_count = parseInteger<int>(fields[9]);
    const auto last_played = parseInteger<std::int64_t>(fields[10]);
    const auto file_size = parseInteger<std::uintmax_t>(fields[11]);
    const auto file_mtime = parseInteger<std::int64_t>(fields[12]);
    const auto fingerprint = hexDecode(fields[13]);
    if (!id || !path || !title || !artist || !album || !genre || !composer || !added_time || !duration
        || !play_count || !last_played || !file_size || !file_mtime || !fingerprint) {
        return std::nullopt;
    }

    app::TrackRecord track{};
    track.id = *id;
    track.path = fs::path{*path};
    track.title = *title;
    track.artist = *artist;
    track.album = *album;
    track.genre = *genre;
    track.composer = *composer;
    track.added_time = *added_time;
    track.duration_seconds = *duration;
    track.play_count = *play_count;
    track.last_played = *last_played;
    track.file_size_bytes = *file_size;
    track.file_mtime_ticks = *file_mtime;
    track.fingerprint = *fingerprint;
    return track;
}

fs::path defaultStorePath()
{
    return fs::temp_directory_path() / "lofibox" / "library-store.tsv";
}

} // namespace

LibraryStore::LibraryStore(std::filesystem::path store_path)
    : store_path_(std::move(store_path))
{
    if (store_path_.empty()) {
        store_path_ = defaultStorePath();
    }
}

const std::filesystem::path& LibraryStore::storePath() const noexcept
{
    return store_path_;
}

LibraryStoreMetadata LibraryStore::metadata() const
{
    return {};
}

app::LibraryModel LibraryStore::load() const
{
    app::LibraryModel model{};
    std::ifstream input(store_path_, std::ios::binary);
    if (!input) {
        return model;
    }
    if (readSchemaVersion(store_path_) != kStoreSchemaVersion) {
        return model;
    }

    std::string line{};
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        if (const auto track = parseTrack(line)) {
            model.tracks.push_back(*track);
        }
    }
    return model;
}

bool LibraryStore::save(const app::LibraryModel& model) const
{
    std::error_code ec{};
    if (!store_path_.parent_path().empty()) {
        fs::create_directories(store_path_.parent_path(), ec);
    }
    if (ec) {
        return false;
    }
    auto temporary_path = store_path_;
    temporary_path += ".tmp";
    std::ofstream output(temporary_path, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }
    output << "# LoFiBox library store schema=" << kStoreSchemaVersion << '\n';
    for (const auto& track : model.tracks) {
        if (!appendTrack(output, track)) {
            output.close();
            fs::remove(temporary_path, ec);
            return false;
        }
    }
    output.flush();
    const bool wrote = static_cast<bool>(output);
    output.close();
    if (!wrote) {
        fs::remove(temporary_path, ec);
        return false;
    }

    fs::rename(temporary_path, store_path_, ec);
    if (ec) {
        // Windows does not replace an existing destination through rename.
        // The temporary file has already been fully written and closed.
        ec.clear();
        fs::remove(store_path_, ec);
        if (ec) {
            fs::remove(temporary_path, ec);
            return false;
        }
        fs::rename(temporary_path, store_path_, ec);
    }
    return !ec;
}

} // namespace lofibox::library
