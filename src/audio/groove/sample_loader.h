// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <string>

#include "audio/groove/sample_buffer.h"

namespace lofibox::audio::groove {

struct SampleLoadResult {
    bool ok{false};
    SampleBuffer buffer{};
    std::string errorMessage{};
};

class SampleLoader {
public:
    [[nodiscard]] SampleLoadResult loadWav(const std::filesystem::path& path) const;
};

} // namespace lofibox::audio::groove
