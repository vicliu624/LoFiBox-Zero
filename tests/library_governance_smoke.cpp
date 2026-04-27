// SPDX-License-Identifier: GPL-3.0-or-later

#include "library/library_governance.h"

#include <cassert>

int main()
{
    lofibox::app::LibraryModel model{};
    model.tracks.push_back({1, "a.mp3", "Alpha", "Artist", "Album", "Genre", "Composer", 10, 180, 3, 100});
    model.tracks.push_back({2, "b.mp3", "Beta", "Artist", "Album", "Genre", "Composer", 20, 181, 9, 50});

    lofibox::library::LibraryGovernanceService service{};
    const auto changes = service.incrementalChanges(model, {"b.mp3", "c.mp3"});
    assert(changes.size() == 2);

    const auto migrations = service.migrationPlan(1, 4);
    assert(migrations.size() == 3);

    const auto most_played = service.smartPlaylist(model, {lofibox::app::PlaylistKind::MostPlayed, 1});
    assert(most_played.front() == 2);

    auto groups = service.groupSearchResults(
        {lofibox::app::mediaItemFromTrackRecord(model.tracks[0])},
        {lofibox::app::MediaItem{.id = "remote-1", .title = "Remote"}});
    assert(groups.size() == 2);
    assert(groups[0].source_label == "LOCAL LIBRARY");

    assert(service.sameRecording({"mbid-1", "", "", "", 0}, {"mbid-1", "", "", "", 0}));
    assert(service.sameRecording({"", "fp", "", "", 0}, {"", "fp", "", "", 0}));
    assert(service.sameRecording({"", "", "Song", "Artist", 200}, {"", "", "Song", "Artist", 201}));
    return 0;
}
