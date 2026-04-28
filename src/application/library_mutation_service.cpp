// SPDX-License-Identifier: GPL-3.0-or-later

#include "application/library_mutation_service.h"

#include <chrono>
#include <utility>

#include "app/library_controller.h"

namespace lofibox::application {

LibraryMutationService::LibraryMutationService(app::LibraryController& controller) noexcept
    : controller_(controller)
{
}

void LibraryMutationService::startLoading() const noexcept
{
    controller_.startLoading();
}

void LibraryMutationService::refreshLibrary(const std::vector<std::filesystem::path>& media_roots, const app::MetadataProvider& metadata_provider) const
{
    controller_.refreshLibrary(media_roots, metadata_provider);
}

void LibraryMutationService::mergeRemoteTracks(const app::RemoteServerProfile& profile, const std::vector<app::RemoteTrack>& tracks) const
{
    controller_.mergeRemoteTracks(profile, tracks);
}

bool LibraryMutationService::applyRemoteTrackFacts(const app::RemoteServerProfile& profile, const app::RemoteTrack& remote_track) const
{
    return controller_.applyRemoteTrackFacts(profile, remote_track);
}

void LibraryMutationService::setSongsContextAll() const
{
    controller_.setSongsContextAll();
}

void LibraryMutationService::setSongsContextTrackIds(std::string label, std::vector<int> ids) const
{
    controller_.setSongsContextTrackIds(std::move(label), std::move(ids));
}

void LibraryMutationService::cycleSongSortMode() const
{
    controller_.cycleSongSortMode();
}

void LibraryMutationService::recordPlaybackStarted(app::TrackRecord& track) const
{
    ++track.play_count;
    track.last_played = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch())
                            .count();
}

} // namespace lofibox::application
