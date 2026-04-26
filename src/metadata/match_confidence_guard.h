// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/runtime_services.h"

namespace lofibox::metadata {

class MatchConfidenceGuard {
public:
    [[nodiscard]] bool acceptsIdentity(const app::TrackIdentity& identity) const noexcept;
    [[nodiscard]] bool acceptsAuthoritativeLyrics(const app::TrackLyrics& lyrics, const app::TrackIdentity& identity) const noexcept;
};

} // namespace lofibox::metadata
