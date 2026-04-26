// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <vector>

#include "app/library_model.h"

namespace lofibox::library {

struct LibraryStoreMetadata {
    int schema_version{1};
};

class LibraryStore {
public:
    explicit LibraryStore(std::filesystem::path store_path = {});

    [[nodiscard]] const std::filesystem::path& storePath() const noexcept;
    [[nodiscard]] LibraryStoreMetadata metadata() const;
    [[nodiscard]] app::LibraryModel load() const;
    bool save(const app::LibraryModel& model) const;

private:
    std::filesystem::path store_path_{};
};

} // namespace lofibox::library
