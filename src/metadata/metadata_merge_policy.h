// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "app/library_model.h"
#include "app/runtime_services.h"

namespace lofibox::metadata {

class MetadataMergePolicy {
public:
    void mergeIntoTrack(app::TrackRecord& track, const app::TrackMetadata& metadata) const;

private:
    [[nodiscard]] static bool shouldReplaceText(const std::string& current, const std::optional<std::string>& candidate);
};

} // namespace lofibox::metadata
