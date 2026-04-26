// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <vector>

#include "app/library_model.h"
#include "app/runtime_services.h"

namespace lofibox::app {

class LibraryRepository {
public:
    [[nodiscard]] LibraryIndexState state() const noexcept;
    [[nodiscard]] const LibraryModel& model() const noexcept;
    [[nodiscard]] LibraryModel& mutableModel() noexcept;

    void markLoading() noexcept;
    void rescan(const std::vector<std::filesystem::path>& media_roots, const MetadataProvider& metadata_provider);

    [[nodiscard]] const TrackRecord* findTrack(int id) const noexcept;
    [[nodiscard]] TrackRecord* findMutableTrack(int id) noexcept;

private:
    LibraryIndexState state_{LibraryIndexState::Uninitialized};
    LibraryModel library_{};
};

} // namespace lofibox::app
