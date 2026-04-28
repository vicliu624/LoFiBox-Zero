// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <vector>

#include "app/library_model.h"
#include "app/runtime_services.h"

namespace lofibox::app {

[[nodiscard]] LibraryModel scanLibrary(const std::vector<std::filesystem::path>& requested_roots, const MetadataProvider& metadata_provider);
void rebuildLibraryIndexes(LibraryModel& model);

} // namespace lofibox::app
