// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <fstream>
#include <iostream>

#include "app/library_controller.h"
#include "app/runtime_services.h"

namespace fs = std::filesystem;

namespace {

void touchFile(const fs::path& path)
{
    std::ofstream output(path.string(), std::ios::binary);
    output << "test";
}

} // namespace

int main()
{
    const fs::path root = fs::temp_directory_path() / "lofibox_zero_library_controller_smoke";
    std::error_code ec{};
    fs::remove_all(root, ec);
    fs::create_directories(root / "ArtistA" / "AlbumA");
    fs::create_directories(root / "ArtistB" / "AlbumB");
    touchFile(root / "ArtistA" / "AlbumA" / "alpha.mp3");
    touchFile(root / "ArtistB" / "AlbumB" / "beta.flac");

    auto services = lofibox::app::withNullRuntimeServices();
    lofibox::app::LibraryController controller{};
    controller.startLoading();
    controller.refreshLibrary({root}, *services.metadata.metadata_provider);

    if (controller.state() != lofibox::app::LibraryIndexState::Ready) {
        std::cerr << "Expected controller to scan into Ready state.\n";
        return 1;
    }

    const auto index_rows = controller.rowsForPage(lofibox::app::AppPage::MusicIndex);
    if (!index_rows || index_rows->size() != 6) {
        std::cerr << "Expected fixed Music Index rows from controller.\n";
        return 1;
    }

    auto open_result = controller.openSelectedListItem(lofibox::app::AppPage::MusicIndex, 2);
    if (open_result.kind != lofibox::app::LibraryOpenResult::Kind::PushPage || open_result.page != lofibox::app::AppPage::Songs) {
        std::cerr << "Expected Music Index Songs row to push Songs page.\n";
        return 1;
    }

    const auto songs_rows = controller.rowsForPage(lofibox::app::AppPage::Songs);
    if (!songs_rows || songs_rows->size() != 2) {
        std::cerr << "Expected Songs rows to expose scanned tracks.\n";
        return 1;
    }
    if ((*songs_rows)[0].first > (*songs_rows)[1].first) {
        std::cerr << "Expected default Songs sort to be title ascending.\n";
        return 1;
    }

    controller.cycleSongSortMode();
    const auto songs_desc_rows = controller.rowsForPage(lofibox::app::AppPage::Songs);
    if (!songs_desc_rows || songs_desc_rows->size() != 2 || (*songs_desc_rows)[0].first < (*songs_desc_rows)[1].first) {
        std::cerr << "Expected F3 sort cycle to switch Songs to title descending.\n";
        return 1;
    }

    open_result = controller.openSelectedListItem(lofibox::app::AppPage::Songs, 0);
    if (open_result.kind != lofibox::app::LibraryOpenResult::Kind::StartTrack || open_result.track_id <= 0) {
        std::cerr << "Expected Songs confirm to yield a track playback target.\n";
        return 1;
    }

    open_result = controller.openSelectedListItem(lofibox::app::AppPage::MusicIndex, 1);
    if (open_result.kind != lofibox::app::LibraryOpenResult::Kind::PushPage || open_result.page != lofibox::app::AppPage::Albums) {
        std::cerr << "Expected Music Index Albums row to push Albums page.\n";
        return 1;
    }

    const auto album_rows = controller.rowsForPage(lofibox::app::AppPage::Albums);
    if (!album_rows || album_rows->empty()) {
        std::cerr << "Expected Albums rows to be owned by controller.\n";
        return 1;
    }

    fs::remove_all(root, ec);
    return 0;
}
