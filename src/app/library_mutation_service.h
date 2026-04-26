// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/library_model.h"

namespace lofibox::app {

class LibraryMutationService {
public:
    void recordPlaybackStarted(TrackRecord& track) const;
};

} // namespace lofibox::app
