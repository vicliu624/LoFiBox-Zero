// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/library_repository.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

class CountingMetadataProvider final : public lofibox::app::MetadataProvider {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "counting test metadata"; }

    [[nodiscard]] lofibox::app::TrackMetadata read(
        const fs::path& path,
        lofibox::app::MetadataReadMode mode) const override
    {
        ++read_count;
        read_modes.push_back(mode);
        const auto stem = path.stem().string();
        return lofibox::app::TrackMetadata{
            .title = "embedded " + stem,
            .artist = "test artist",
            .album = "test album",
            .genre = "ambient",
            .composer = "test composer",
            .duration_seconds = 222};
    }

    mutable int read_count{0};
    mutable std::vector<lofibox::app::MetadataReadMode> read_modes{};
};

void writeFile(const fs::path& path, std::string_view contents, bool append = false)
{
    std::ofstream output(
        path,
        std::ios::binary | (append ? std::ios::app : std::ios::trunc));
    assert(output);
    output << contents;
    assert(output);
}

const lofibox::app::TrackRecord& trackAt(
    const lofibox::app::LibraryModel& model,
    const fs::path& path)
{
    for (const auto& track : model.tracks) {
        if (track.path.filename() == path.filename()) {
            return track;
        }
    }
    assert(false && "expected local track was missing");
    return model.tracks.front();
}

void assertLocalOnly(const CountingMetadataProvider& provider)
{
    for (const auto mode : provider.read_modes) {
        assert(mode == lofibox::app::MetadataReadMode::LocalOnly);
    }
}

} // namespace

int main()
{
    const fs::path root = fs::temp_directory_path() / "lofibox_library_incremental_scan_smoke";
    std::error_code ec{};
    fs::remove_all(root, ec);
    fs::create_directories(root, ec);
    assert(!ec);

    const auto first_path = root / "first.mp3";
    const auto second_path = root / "second.flac";
    writeFile(first_path, "first audio bytes");
    writeFile(second_path, "second audio bytes");
    writeFile(root / "cover.jpg", "not an audio candidate");

    const auto store_path = root / "state" / "library-index.tsv";
    CountingMetadataProvider initial_provider{};
    lofibox::app::LibraryRepository initial{};
    initial.configureIndexStorePath(store_path);
    initial.rescan({root}, initial_provider);
    assert(initial_provider.read_count == 2);
    assertLocalOnly(initial_provider);
    assert(initial.model().tracks.size() == 2U);
    const int first_id = trackAt(initial.model(), first_path).id;
    assert(trackAt(initial.model(), first_path).title == "embedded first");
    assert(trackAt(initial.model(), first_path).file_size_bytes > 0U);
    assert(fs::exists(store_path));

    // A new repository simulates an app restart. Unchanged local tracks
    // retain their stored tags and never launch a metadata helper again.
    CountingMetadataProvider restarted_provider{};
    lofibox::app::LibraryRepository restarted{};
    restarted.configureIndexStorePath(store_path);
    assert(restarted.model().tracks.size() == 2U);
    restarted.rescan({root}, restarted_provider);
    assert(restarted_provider.read_count == 0);
    assert(restarted.model().tracks.size() == 2U);
    assert(trackAt(restarted.model(), first_path).id == first_id);
    assert(trackAt(restarted.model(), first_path).title == "embedded first");

    // A changed fingerprint rereads exactly that one file, while an added
    // candidate is read once and non-audio files remain outside the scan.
    writeFile(first_path, " changed", true);
    restarted.rescan({root}, restarted_provider);
    assert(restarted_provider.read_count == 1);
    assert(trackAt(restarted.model(), first_path).id == first_id);

    const auto third_path = root / "third.ogg";
    writeFile(third_path, "third audio bytes");
    restarted.rescan({root}, restarted_provider);
    assert(restarted_provider.read_count == 2);
    assert(restarted.model().tracks.size() == 3U);
    assert(trackAt(restarted.model(), third_path).title == "embedded third");

    fs::remove(second_path, ec);
    assert(!ec);
    restarted.rescan({root}, restarted_provider);
    assert(restarted_provider.read_count == 2);
    assert(restarted.model().tracks.size() == 2U);

    fs::remove_all(root, ec);
    return 0;
}
