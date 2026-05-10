// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <string>

#include "audio/groove/sample_buffer.h"

namespace lofibox::audio::groove {

struct WavExportResult {
    bool ok{false};
    std::filesystem::path path{};
    std::string errorMessage{};
};

class WavExporter {
public:
    [[nodiscard]] WavExportResult writePcm16(const std::filesystem::path& path, const SampleBuffer& buffer, bool normalize) const;
};

} // namespace lofibox::audio::groove
