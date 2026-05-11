// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/groove/offline_groove_renderer.h"
#include "audio/groove/groove_export_service.h"
#include "audio/groove/media_segment_decoder.h"
#include "audio/groove/sample_editor.h"
#include "audio/groove/sample_loader.h"
#include "audio/groove/wav_exporter.h"
#include "groove/groove_project.h"
#include "media_fixture_utils.h"

#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] std::optional<std::filesystem::path> resolveFfmpeg()
{
#if defined(_WIN32)
    return test_media_fixture::resolveExecutablePath(L"FFMPEG_PATH", L"ffmpeg.exe");
#elif defined(__linux__)
    return test_media_fixture::resolveExecutablePath("FFMPEG_PATH", "ffmpeg");
#else
    return std::nullopt;
#endif
}

[[nodiscard]] bool makeFixture(
    const std::filesystem::path& ffmpeg,
    const std::filesystem::path& source_wav,
    const std::filesystem::path& target_path,
    std::vector<std::string> codec_args)
{
    std::vector<std::string> args{
        "-hide_banner",
        "-loglevel",
        "error",
        "-nostdin",
        "-i",
        source_wav.string(),
    };
    args.insert(args.end(), codec_args.begin(), codec_args.end());
    args.emplace_back("-y");
    args.emplace_back(target_path.string());
    return test_media_fixture::runCommand(ffmpeg, args) && std::filesystem::exists(target_path);
}

} // namespace

int main()
{
    lofibox::audio::groove::SampleBuffer buffer{};
    buffer.sampleRate = 1000;
    buffer.channels = 1;
    buffer.samples.assign(1000, 0.0f);
    buffer.samples[100] = 0.25f;
    buffer.samples[500] = 0.75f;
    buffer.samples[800] = -0.5f;

    lofibox::audio::groove::SampleEditor editor{};
    const auto trimmed = editor.trim(buffer, 0.1, 0.6);
    assert(trimmed.ok);
    assert(trimmed.buffer.frameCount() == 500);

    const auto normalized = editor.normalize(trimmed.buffer, 1.0f);
    assert(normalized.ok);
    float peak = 0.0f;
    for (float sample : normalized.buffer.samples) {
        peak = std::max(peak, std::abs(sample));
    }
    assert(peak > 0.99f);

    const auto faded = editor.fadeIn(normalized.buffer, 20.0);
    assert(faded.ok);
    assert(std::abs(faded.buffer.samples.front()) < 0.001f);

    const auto reversed = editor.reverse(buffer);
    assert(reversed.ok);
    assert(reversed.buffer.samples[199] == -0.5f);

    const auto slices = editor.autoSlice(buffer, 8);
    assert(!slices.empty());

    auto project = lofibox::groove::makeDefaultGrooveProject("Render");
    project.bpm = 120;
    project.activePattern = 0;
    project.exportSettings.target = lofibox::groove::GrooveExportTarget::CurrentPattern;
    project.exportSettings.sampleRate = 1000;
    project.exportSettings.normalize = false;
    project.exportSettings.includeTail = false;
    project.patterns[0].tracks[0].steps[0].trigger = true;
    project.patterns[0].tracks[0].steps[4].trigger = true;

    lofibox::audio::groove::GrooveSampleBank bank{};
    bank.slots[0] = trimmed.buffer;
    lofibox::audio::groove::OfflineGrooveRenderer renderer{};
    const auto rendered = renderer.render(project, bank);
    assert(rendered.sampleRate == 1000);
    assert(rendered.channels == 2);
    assert(rendered.frameCount() == 2000);

    const auto wav = std::filesystem::temp_directory_path() / "lofibox-groove-audio-smoke.wav";
    lofibox::audio::groove::WavExporter exporter{};
    const auto exported = exporter.writePcm16(wav, rendered, false);
    assert(exported.ok);

    lofibox::audio::groove::SampleLoader loader{};
    const auto loaded = loader.loadWav(wav);
    assert(loaded.ok);
    assert(loaded.buffer.sampleRate == 1000);
    assert(loaded.buffer.channels == 2);
    assert(loaded.buffer.frameCount() == rendered.frameCount());

    const auto sample_wav = std::filesystem::temp_directory_path() / "lofibox-groove-export-sample.wav";
    const auto sample_exported = exporter.writePcm16(sample_wav, trimmed.buffer, false);
    assert(sample_exported.ok);

    if (const auto ffmpeg = resolveFfmpeg()) {
        const auto run_id = std::chrono::steady_clock::now().time_since_epoch().count();
        const auto media_root = std::filesystem::temp_directory_path() / ("lofibox-groove-capture-formats-" + std::to_string(run_id));
        std::error_code ec{};
        std::filesystem::remove_all(media_root, ec);
        std::filesystem::create_directories(media_root, ec);
        assert(!ec);

        constexpr double pi = 3.14159265358979323846;
        lofibox::audio::groove::SampleBuffer capture_source{};
        capture_source.sampleRate = 48000;
        capture_source.channels = 2;
        capture_source.samples.reserve(48000U * 2U * 2U);
        for (int frame = 0; frame < 48000 * 2; ++frame) {
            const auto value = static_cast<float>(std::sin((2.0 * pi * 440.0 * static_cast<double>(frame)) / 48000.0) * 0.35);
            capture_source.samples.push_back(value);
            capture_source.samples.push_back(value);
        }
        const auto capture_source_wav = media_root / "capture-source-master.wav";
        const auto capture_source_export = exporter.writePcm16(capture_source_wav, capture_source, false);
        assert(capture_source_export.ok);

        const std::vector<std::pair<std::string, std::vector<std::string>>> formats{
            {"wav", {}},
            {"mp3", {"-c:a", "libmp3lame"}},
            {"flac", {"-c:a", "flac"}},
            {"ogg", {"-c:a", "libvorbis"}},
            {"aac", {"-c:a", "aac", "-f", "adts"}},
        };

        lofibox::audio::groove::MediaSegmentDecoder segment_decoder{};
        int decoded_formats = 0;
        for (const auto& [extension, codec_args] : formats) {
            const auto fixture = media_root / ("capture-source." + extension);
            if (extension == "wav") {
                std::filesystem::copy_file(capture_source_wav, fixture, std::filesystem::copy_options::overwrite_existing, ec);
                assert(!ec);
            } else {
                assert(makeFixture(*ffmpeg, capture_source_wav, fixture, codec_args));
            }

            const auto decoded = segment_decoder.decodeSegment(fixture.string(), 0.25, 0.50);
            if (!decoded.has_value()) {
                std::cerr << "Expected Groove capture decode for ." << extension
                          << " but got: " << segment_decoder.lastErrorMessage() << "\n";
                return 1;
            }
            assert(segment_decoder.lastErrorMessage().empty());
            assert(decoded->sampleRate == 48000);
            assert(decoded->channels == 2);
            assert(decoded->frameCount() > 0U);
            if (decoded->durationSeconds() <= 0.40 || decoded->durationSeconds() >= 0.65) {
                std::cerr << "Unexpected decode duration for ." << extension
                          << ": " << decoded->durationSeconds() << "\n";
                return 1;
            }
            ++decoded_formats;
        }
        assert(decoded_formats == 5);
        std::filesystem::remove_all(media_root, ec);
    } else {
        std::cout << "ffmpeg not found; skipping Groove multi-format capture decoder smoke.\n";
    }

    project.sounds[0].type = lofibox::groove::GrooveSoundType::UserSample;
    project.sounds[0].sourceUri = sample_wav.string();
    project.sounds[0].name = "KIK";
    project.songChain.enabled = true;
    project.songChain.items = {lofibox::groove::GrooveSongChainItem{0, 2, false, 0, "BEAT"}};
    project.exportSettings.target = lofibox::groove::GrooveExportTarget::SongChain;
    project.exportSettings.includeTail = true;
    project.exportSettings.tailSeconds = 0.25;
    lofibox::groove::GrooveStoragePaths paths{};
    paths.exportsDir = std::filesystem::temp_directory_path() / "lofibox-groove-export-primary";
    paths.fallbackExportsDir = std::filesystem::temp_directory_path() / "lofibox-groove-export-fallback";
    lofibox::audio::groove::GrooveExportService export_service{};
    const auto export_result = export_service.exportProject(project, paths);
    assert(export_result.ok);
    assert(std::filesystem::exists(export_result.path));
    assert(std::filesystem::file_size(export_result.path) > 44U);
    const auto exported_loaded = loader.loadWav(export_result.path);
    assert(exported_loaded.ok);
    assert(exported_loaded.buffer.durationSeconds() >= 4.0);

    return 0;
}
