// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/library_repository.h"

#include "app/library_query_service.h"
#include "app/library_scanner.h"

namespace lofibox::app {

LibraryIndexState LibraryRepository::state() const noexcept
{
    return state_;
}

const LibraryModel& LibraryRepository::model() const noexcept
{
    return library_;
}

LibraryModel& LibraryRepository::mutableModel() noexcept
{
    return library_;
}

void LibraryRepository::markLoading() noexcept
{
    state_ = LibraryIndexState::Loading;
}

void LibraryRepository::rescan(const std::vector<std::filesystem::path>& media_roots, const MetadataProvider& metadata_provider)
{
    library_ = scanLibrary(media_roots, metadata_provider);
    state_ = library_.degraded ? LibraryIndexState::Degraded : LibraryIndexState::Ready;
}

const TrackRecord* LibraryRepository::findTrack(int id) const noexcept
{
    return LibraryQueryService::findTrack(library_, id);
}

TrackRecord* LibraryRepository::findMutableTrack(int id) noexcept
{
    return LibraryQueryService::findMutableTrack(library_, id);
}

} // namespace lofibox::app
