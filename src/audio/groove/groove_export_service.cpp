// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/groove/groove_export_service.h"

#include <chrono>
#include <ctime>
#include <sstream>

#include "audio/groove/offline_groove_renderer.h"
#include "audio/groove/sample_loader.h"
#include "audio/groove/wav_exporter.h"

namespace lofibox::audio::groove {
namespace {

[[nodiscard]] std::filesystem::path pathFromUri(std::string_view uri)
{
    constexpr std::string_view kFilePrefix{"file://"};
    if (uri.starts_with(kFilePrefix)) {
        return std::filesystem::path{std::string{uri.substr(kFilePrefix.size())}};
    }
    return std::filesystem::path{std::string{uri}};
}

[[nodiscard]] std::string timestampSuffix()
{
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    char buffer[32]{};
    std::strftime(buffer, sizeof(buffer), "%Y%m%d-%H%M%S", &tm);
    return buffer;
}

[[nodiscard]] std::filesystem::path exportPath(
    const lofibox::groove::GrooveProject& project,
    const std::filesystem::path& dir)
{
    const auto name = project.name.empty() ? std::string{"Groove"} : project.name;
    return dir / (lofibox::groove::sanitizeProjectFileName(name) + "-" + timestampSuffix() + ".wav");
}

} // namespace

GrooveSampleBank GrooveExportService::buildSampleBank(const lofibox::groove::GrooveProject& project) const
{
    GrooveSampleBank bank{};
    SampleLoader loader{};
    for (std::size_t slot = 0; slot < project.sounds.size(); ++slot) {
        const auto& sound = project.sounds[slot];
        if (sound.sourceUri.empty()) {
            continue;
        }
        const auto loaded = loader.loadWav(pathFromUri(sound.sourceUri));
        if (loaded.ok) {
            bank.slots[slot] = loaded.buffer;
        }
    }
    return bank;
}

GrooveExportServiceResult GrooveExportService::exportProject(
    const lofibox::groove::GrooveProject& project,
    const lofibox::groove::GrooveStoragePaths& paths) const
{
    const auto bank = buildSampleBank(project);
    OfflineGrooveRenderer renderer{};
    const auto buffer = renderer.render(project, bank);

    WavExporter exporter{};
    auto primary_path = exportPath(project, paths.exportsDir);
    auto result = exporter.writePcm16(primary_path, buffer, project.exportSettings.normalize);
    if (!result.ok && paths.fallbackExportsDir != paths.exportsDir) {
        auto fallback_path = exportPath(project, paths.fallbackExportsDir);
        result = exporter.writePcm16(fallback_path, buffer, project.exportSettings.normalize);
    }
    if (!result.ok) {
        return {false, result.path, 0, buffer.durationSeconds(), result.errorMessage};
    }
    return {true, result.path, 100, buffer.durationSeconds(), {}};
}

} // namespace lofibox::audio::groove
