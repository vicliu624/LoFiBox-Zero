// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/groove/offline_groove_renderer.h"
#include "audio/groove/sample_editor.h"
#include "audio/groove/sample_loader.h"
#include "audio/groove/wav_exporter.h"
#include "groove/groove_project.h"

#include <cassert>
#include <cmath>
#include <filesystem>

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

    return 0;
}
