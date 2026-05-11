// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "groove/groove_project.h"
#include "groove/groove_project_repository.h"

namespace lofibox::application {

struct GrooveCaptureSource {
    bool available{false};
    std::string sourceTrackId{};
    std::string sourceUri{};
    std::string displayName{};
    double positionSeconds{0.0};
};

struct GrooveCaptureOperation {
    GrooveCaptureSource source{};
    std::uint8_t targetSoundSlot{0};
    double durationSeconds{0.0};
    bool normalize{true};
    double fadeInMs{2.0};
    double fadeOutMs{4.0};
};

struct GrooveOperationResult {
    bool ok{false};
    std::string status{};
    std::string errorMessage{};
    std::filesystem::path path{};
    int progressPercent{0};
    double durationSeconds{0.0};
};

class GrooveCommandService {
public:
    GrooveCommandService();
    explicit GrooveCommandService(lofibox::groove::GrooveProjectRepository repository);

    [[nodiscard]] const lofibox::groove::GrooveProjectRepository& repository() const noexcept;

    [[nodiscard]] GrooveOperationResult captureToSlot(
        lofibox::groove::GrooveProject& project,
        const GrooveCaptureOperation& operation) const;
    [[nodiscard]] GrooveOperationResult rewriteSample(
        lofibox::groove::GrooveProject& project,
        std::uint8_t sound_slot,
        bool reverse) const;
    [[nodiscard]] GrooveOperationResult autoSlice(
        lofibox::groove::GrooveProject& project,
        std::uint8_t sound_slot,
        std::uint8_t max_slices) const;
    [[nodiscard]] GrooveOperationResult exportProject(const lofibox::groove::GrooveProject& project) const;
    [[nodiscard]] GrooveOperationResult renderPreview(const lofibox::groove::GrooveProject& project) const;
    [[nodiscard]] GrooveOperationResult saveProject(const lofibox::groove::GrooveProject& project) const;
    [[nodiscard]] GrooveOperationResult loadFirstProject(lofibox::groove::GrooveProject& project) const;
    [[nodiscard]] GrooveOperationResult newProject(lofibox::groove::GrooveProject& project) const;
    [[nodiscard]] GrooveOperationResult deleteProject(lofibox::groove::GrooveProject& project) const;

private:
    lofibox::groove::GrooveProjectRepository repository_{};
};

} // namespace lofibox::application
