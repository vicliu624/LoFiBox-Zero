// SPDX-License-Identifier: GPL-3.0-or-later

#include "audio/groove/sample_editor.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace lofibox::audio::groove {
namespace {

[[nodiscard]] SampleEditResult ok(SampleBuffer buffer)
{
    return SampleEditResult{true, std::move(buffer), {}};
}

[[nodiscard]] SampleEditResult fail(std::string message)
{
    return SampleEditResult{false, {}, std::move(message)};
}

[[nodiscard]] float frameEnergy(const SampleBuffer& input, std::size_t frame)
{
    float energy = 0.0f;
    for (int channel = 0; channel < input.channels; ++channel) {
        const float value = input.sample(frame, channel);
        energy += value * value;
    }
    return energy / static_cast<float>(std::max(1, input.channels));
}

} // namespace

SampleEditResult SampleEditor::trim(const SampleBuffer& input, double start_seconds, double end_seconds) const
{
    if (input.sampleRate <= 0 || input.channels <= 0 || input.samples.empty()) {
        return fail("input sample is empty");
    }
    if (end_seconds <= start_seconds) {
        return fail("trim end must be after start");
    }
    const auto start_frame = static_cast<std::size_t>(std::floor(std::max(0.0, start_seconds) * static_cast<double>(input.sampleRate)));
    const auto end_frame = static_cast<std::size_t>(std::ceil(std::max(0.0, end_seconds) * static_cast<double>(input.sampleRate)));
    if (start_frame >= input.frameCount()) {
        return fail("trim start is outside sample");
    }
    const auto clamped_end = std::min(end_frame, input.frameCount());
    if (clamped_end <= start_frame) {
        return fail("trim range is empty");
    }
    SampleBuffer output{};
    output.sampleRate = input.sampleRate;
    output.channels = input.channels;
    output.samples.reserve((clamped_end - start_frame) * static_cast<std::size_t>(input.channels));
    const auto begin = input.samples.begin() + static_cast<std::ptrdiff_t>(start_frame * static_cast<std::size_t>(input.channels));
    const auto end = input.samples.begin() + static_cast<std::ptrdiff_t>(clamped_end * static_cast<std::size_t>(input.channels));
    output.samples.assign(begin, end);
    return ok(std::move(output));
}

SampleEditResult SampleEditor::normalize(const SampleBuffer& input, float target_peak) const
{
    float peak = 0.0f;
    for (float sample : input.samples) {
        peak = std::max(peak, std::abs(sample));
    }
    if (peak <= 0.000001f) {
        return ok(input);
    }
    SampleBuffer output = input;
    const float gain = std::max(0.0f, target_peak) / peak;
    for (float& sample : output.samples) {
        sample = std::clamp(sample * gain, -1.0f, 1.0f);
    }
    return ok(std::move(output));
}

SampleEditResult SampleEditor::fadeIn(const SampleBuffer& input, double fade_ms) const
{
    SampleBuffer output = input;
    const auto fade_frames = std::min<std::size_t>(
        output.frameCount(),
        static_cast<std::size_t>(std::ceil(std::max(0.0, fade_ms) * static_cast<double>(output.sampleRate) / 1000.0)));
    if (fade_frames == 0U) {
        return ok(std::move(output));
    }
    for (std::size_t frame = 0; frame < fade_frames; ++frame) {
        const float gain = static_cast<float>(frame) / static_cast<float>(std::max<std::size_t>(1, fade_frames - 1U));
        for (int channel = 0; channel < output.channels; ++channel) {
            output.setSample(frame, channel, output.sample(frame, channel) * gain);
        }
    }
    return ok(std::move(output));
}

SampleEditResult SampleEditor::fadeOut(const SampleBuffer& input, double fade_ms) const
{
    SampleBuffer output = input;
    const auto fade_frames = std::min<std::size_t>(
        output.frameCount(),
        static_cast<std::size_t>(std::ceil(std::max(0.0, fade_ms) * static_cast<double>(output.sampleRate) / 1000.0)));
    if (fade_frames == 0U) {
        return ok(std::move(output));
    }
    const auto total = output.frameCount();
    for (std::size_t frame = 0; frame < fade_frames; ++frame) {
        const float gain = 1.0f - (static_cast<float>(frame) / static_cast<float>(std::max<std::size_t>(1, fade_frames - 1U)));
        const auto target_frame = (total - fade_frames) + frame;
        for (int channel = 0; channel < output.channels; ++channel) {
            output.setSample(target_frame, channel, output.sample(target_frame, channel) * gain);
        }
    }
    return ok(std::move(output));
}

SampleEditResult SampleEditor::reverse(const SampleBuffer& input) const
{
    SampleBuffer output = input;
    const auto frames = output.frameCount();
    for (std::size_t left = 0; left < frames / 2U; ++left) {
        const auto right = frames - 1U - left;
        for (int channel = 0; channel < output.channels; ++channel) {
            const float a = output.sample(left, channel);
            const float b = output.sample(right, channel);
            output.setSample(left, channel, b);
            output.setSample(right, channel, a);
        }
    }
    return ok(std::move(output));
}

std::vector<lofibox::groove::SampleSlice> SampleEditor::autoSlice(const SampleBuffer& input, std::uint8_t max_slices) const
{
    std::vector<lofibox::groove::SampleSlice> slices{};
    if (input.frameCount() == 0U || max_slices == 0U) {
        return slices;
    }

    std::vector<std::size_t> cuts{0};
    const std::size_t window = static_cast<std::size_t>(std::max(64, input.sampleRate / 200));
    const std::size_t min_gap = static_cast<std::size_t>(std::max(1, input.sampleRate / 12));
    float previous = 0.0f;
    for (std::size_t frame = window; frame + window < input.frameCount(); frame += window) {
        float energy = 0.0f;
        for (std::size_t offset = 0; offset < window; ++offset) {
            energy += frameEnergy(input, frame + offset);
        }
        energy /= static_cast<float>(window);
        const bool transient = energy > 0.0005f && energy > previous * 2.4f;
        if (transient && cuts.size() < max_slices && frame - cuts.back() >= min_gap) {
            cuts.push_back(frame);
        }
        previous = (previous * 0.80f) + (energy * 0.20f);
    }
    cuts.push_back(input.frameCount());

    for (std::size_t index = 0; index + 1U < cuts.size() && slices.size() < max_slices; ++index) {
        lofibox::groove::SampleSlice slice{};
        slice.id = "slice-" + std::to_string(index + 1U);
        slice.name = "S" + (index + 1U < 10U ? std::string{"0"} : std::string{}) + std::to_string(index + 1U);
        slice.startSeconds = static_cast<double>(cuts[index]) / static_cast<double>(input.sampleRate);
        slice.endSeconds = static_cast<double>(cuts[index + 1U]) / static_cast<double>(input.sampleRate);
        slices.push_back(std::move(slice));
    }
    return slices;
}

} // namespace lofibox::audio::groove
