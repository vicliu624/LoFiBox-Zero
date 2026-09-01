// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/library_repository.h"

#include "library/library_governance.h"
#include "library/library_indexer.h"

#include <utility>

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

const std::vector<::lofibox::library::LibraryFileChange>& LibraryRepository::lastChanges() const noexcept
{
    return last_changes_;
}

const std::vector<::lofibox::library::LibraryMigration>& LibraryRepository::migrationPlan() const noexcept
{
    return migration_plan_;
}

void LibraryRepository::configureIndexStorePath(std::filesystem::path store_path)
{
    if (store_path.empty()) {
        return;
    }

    index_store_.emplace(std::move(store_path));
    auto persisted = index_store_->load();
    if (persisted.tracks.empty()) {
        return;
    }
    rebuildLibraryIndexes(persisted);
    library_ = std::move(persisted);
}

void LibraryRepository::markLoading() noexcept
{
    state_ = LibraryIndexState::Loading;
}

void LibraryRepository::markStale() noexcept
{
    state_ = LibraryIndexState::Uninitialized;
}

void LibraryRepository::markDegraded() noexcept
{
    library_.degraded = true;
    state_ = LibraryIndexState::Degraded;
}

void LibraryRepository::rescan(
    const std::vector<std::filesystem::path>& media_roots,
    const MetadataProvider& metadata_provider,
    LibraryScanProgressCallback progress)
{
    applyRescanModel(rebuildModel(media_roots, metadata_provider, std::move(progress)));
}

LibraryModel LibraryRepository::rebuildModel(
    const std::vector<std::filesystem::path>& media_roots,
    const MetadataProvider& metadata_provider,
    LibraryScanProgressCallback progress) const
{
    // A stored model is a stable snapshot owned by the scan worker.  Passing
    // the live UI model into that worker would race with remote-library
    // updates, so only durable local records participate in reuse.
    LibraryModel persisted{};
    const LibraryModel* previous_model = nullptr;
    if (index_store_) {
        persisted = index_store_->load();
        previous_model = &persisted;
    }
    return library::LibraryIndexer{}.rebuild(media_roots, metadata_provider, std::move(progress), previous_model);
}

void LibraryRepository::applyRescanModel(LibraryModel next)
{
    std::vector<std::filesystem::path> current_files{};
    current_files.reserve(next.tracks.size());
    for (const auto& track : next.tracks) {
        current_files.push_back(track.path);
    }
    library::LibraryGovernanceService governance{};
    // The old model remains valid until the final move below. Avoid copying
    // every path and metadata string on the UI thread when an async scan
    // completes.
    last_changes_ = governance.incrementalChanges(library_, current_files);
    migration_plan_ = governance.migrationPlan(1, 1);
    library_ = std::move(next);
    state_ = library_.degraded ? LibraryIndexState::Degraded : LibraryIndexState::Ready;
    if (index_store_) {
        (void)index_store_->save(library_);
    }
}

void LibraryRepository::rebuildDerivedIndexes()
{
    rebuildLibraryIndexes(library_);
}

const TrackRecord* LibraryRepository::findTrack(int id) const noexcept
{
    for (const auto& track : library_.tracks) {
        if (track.id == id) {
            return &track;
        }
    }
    return nullptr;
}

TrackRecord* LibraryRepository::findMutableTrack(int id) noexcept
{
    for (auto& track : library_.tracks) {
        if (track.id == id) {
            return &track;
        }
    }
    return nullptr;
}

} // namespace lofibox::app
