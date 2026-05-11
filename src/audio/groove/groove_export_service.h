// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <string>

#include "audio/groove/groove_render_engine.h"
#include "groove/groove_project.h"
#include "groove/groove_project_repository.h"

namespace lofibox::audio::groove {

struct GrooveExportServiceResult {
    bool ok{false};
    std::filesystem::path path{};
    int progressPercent{0};
    double durationSeconds{0.0};
    std::string errorMessage{};
};

class GrooveExportService {
public:
    [[nodiscard]] GrooveExportServiceResult exportProject(
        const lofibox::groove::GrooveProject& project,
        const lofibox::groove::GrooveStoragePaths& paths) const;

    [[nodiscard]] GrooveSampleBank buildSampleBank(const lofibox::groove::GrooveProject& project) const;
};

} // namespace lofibox::audio::groove
