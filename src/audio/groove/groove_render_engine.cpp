// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/groove/groove_render_engine.h"

#include <algorithm>
#include <cmath>

#include "audio/groove/groove_mixer.h"
#include "audio/groove/punch_fx_processor.h"
#include "groove/groove_sequencer.h"

namespace lofibox::audio::groove {
namespace {

void applyNormalizeIfNeeded(SampleBuffer& buffer, bool normalize)
{
    if (!normalize) {
        return;
    }
    float peak = 0.0f;
    for (float sample : buffer.samples) {
        peak = std::max(peak, std::abs(sample));
    }
    if (peak <= 0.000001f) {
        return;
    }
    const float gain = 0.95f / peak;
    for (float& sample : buffer.samples) {
        sample = std::clamp(sample * gain, -1.0f, 1.0f);
    }
}

void renderEventsInto(
    SampleBuffer& output,
    const std::vector<lofibox::groove::GrooveTriggerEvent>& events,
    const GrooveSampleBank& samples,
    const SampleBuffer& fallback)
{
    GrooveMixer mixer{};
    PunchFxProcessor fx{};
    for (const auto& event : events) {
        const auto slot_index = std::min<std::size_t>(event.soundSlot, samples.slots.size() - 1U);
        SampleBuffer voice = samples.slots[slot_index].value_or(fallback);
        const float velocity_gain = static_cast<float>(event.velocity) / 127.0f;
        if (event.fxType != 0U) {
            fx.process(voice, static_cast<PunchFxType>(std::min<std::uint8_t>(event.fxType, 8)), event.fxAmount);
        }
        const auto start_frame = static_cast<std::size_t>(std::max(0.0, event.startSeconds) * static_cast<double>(output.sampleRate));
        mixer.mixInto(output, voice, start_frame, event.gain * velocity_gain, event.pan);
    }
    mixer.clamp(output);
}

} // namespace

SampleBuffer GrooveRenderEngine::renderPattern(
    const lofibox::groove::GrooveProject& project,
    std::uint8_t pattern_index,
    const GrooveSampleBank& samples,
    const lofibox::groove::GrooveExportSettings& settings) const
{
    const auto safe_pattern = std::min<std::uint8_t>(pattern_index, static_cast<std::uint8_t>(project.patterns.size() - 1U));
    double duration = lofibox::groove::patternDurationSeconds(project.patterns[safe_pattern], project.bpm);
    if (settings.includeTail) {
        duration += settings.tailSeconds;
    }
    SampleBuffer output = makeSilentSampleBuffer(static_cast<int>(settings.sampleRate), 2, duration);
    const auto fallback = fallbackClick(static_cast<int>(settings.sampleRate));
    const auto events = lofibox::groove::collectPatternTriggers(project.patterns[safe_pattern], safe_pattern, project.bpm, project.swing);
    renderEventsInto(output, events, samples, fallback);
    applyNormalizeIfNeeded(output, settings.normalize);
    return output;
}

SampleBuffer GrooveRenderEngine::renderSongChain(
    const lofibox::groove::GrooveProject& project,
    const GrooveSampleBank& samples,
    const lofibox::groove::GrooveExportSettings& settings) const
{
    double duration = lofibox::groove::songChainDurationSeconds(project);
    if (settings.includeTail) {
        duration += settings.tailSeconds;
    }
    SampleBuffer output = makeSilentSampleBuffer(static_cast<int>(settings.sampleRate), 2, duration);
    const auto fallback = fallbackClick(static_cast<int>(settings.sampleRate));
    const auto events = lofibox::groove::collectSongChainTriggers(project);
    renderEventsInto(output, events, samples, fallback);
    applyNormalizeIfNeeded(output, settings.normalize);
    return output;
}

SampleBuffer GrooveRenderEngine::fallbackClick(int sample_rate) const
{
    SampleBuffer click = makeSilentSampleBuffer(sample_rate, 1, 0.04);
    for (std::size_t frame = 0; frame < click.frameCount(); ++frame) {
        const float phase = static_cast<float>(frame) / static_cast<float>(std::max<std::size_t>(1, click.frameCount()));
        click.samples[frame] = (1.0f - phase) * 0.55f;
    }
    return click;
}

} // namespace lofibox::audio::groove
