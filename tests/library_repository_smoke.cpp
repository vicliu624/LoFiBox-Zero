// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/library_repository.h"

#include <cassert>

int main()
{
    lofibox::app::LibraryRepository repository{};
    assert(repository.state() == lofibox::app::LibraryIndexState::Uninitialized);

    repository.markLoading();
    assert(repository.state() == lofibox::app::LibraryIndexState::Loading);

    auto& model = repository.mutableModel();
    model.tracks.push_back(lofibox::app::TrackRecord{});
    model.tracks.back().id = 42;
    model.tracks.back().title = "Track";

    const auto* track = repository.findTrack(42);
    assert(track != nullptr);
    assert(track->title == "Track");

    auto* mutable_track = repository.findMutableTrack(42);
    assert(mutable_track != nullptr);
    mutable_track->title = "Changed";
    assert(repository.findTrack(42)->title == "Changed");
    return 0;
}
