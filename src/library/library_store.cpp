// SPDX-License-Identifier: GPL-3.0-or-later

#include "library/library_store.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace lofibox::library {
namespace {

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
    std::string line{};
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        std::istringstream fields(line);
        std::string id{};
        std::string title{};
        std::string artist{};
        if (std::getline(fields, id, '\t') && std::getline(fields, title, '\t') && std::getline(fields, artist, '\t')) {
            app::TrackRecord track{};
            track.id = std::stoi(id);
            track.title = title;
            track.artist = artist;
            model.tracks.push_back(std::move(track));
        }
    }
    return model;
}

bool LibraryStore::save(const app::LibraryModel& model) const
{
    std::error_code ec{};
    fs::create_directories(store_path_.parent_path(), ec);
    if (ec) {
        return false;
    }
    std::ofstream output(store_path_, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }
    output << "# LoFiBox library store schema=1\n";
    for (const auto& track : model.tracks) {
        output << track.id << '\t' << track.title << '\t' << track.artist << '\n';
    }
    return static_cast<bool>(output);
}

} // namespace lofibox::library
