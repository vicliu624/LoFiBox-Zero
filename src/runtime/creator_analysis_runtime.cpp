// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/creator_analysis_runtime.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numeric>
#include <string>
#include <utility>

namespace lofibox::runtime {
namespace {

double clamp01(double value) noexcept
{
    return std::clamp(value, 0.0, 1.0);
}

double bandsEnergy(const std::array<float, 10>& bands) noexcept
{
    double sum = 0.0;
    for (const auto band : bands) {
        sum += clamp01(static_cast<double>(band));
    }
    return sum / static_cast<double>(bands.size());
}

double bandsPeak(const std::array<float, 10>& bands) noexcept
{
    double peak = 0.0;
    for (const auto band : bands) {
        peak = std::max(peak, clamp01(static_cast<double>(band)));
    }
    return peak;
}

double spectralCentroid(const std::array<float, 10>& bands) noexcept
{
    double weighted = 0.0;
    double total = 0.0;
    for (std::size_t index = 0; index < bands.size(); ++index) {
        const double energy = clamp01(static_cast<double>(bands[index]));
        weighted += energy * static_cast<double>(index);
        total += energy;
    }
    return total <= 0.0001 ? 0.0 : weighted / total;
}

std::string estimateKey(const std::array<float, 10>& bands)
{
    constexpr std::array<std::string_view, 12> kCircleOfFifths{{
        "C", "G", "D", "A", "E", "B", "F#", "C#", "G#", "D#", "A#", "F",
    }};
    const auto centroid = spectralCentroid(bands);
    const auto pitch_index = static_cast<std::size_t>(std::clamp(static_cast<int>(std::round((centroid / 9.0) * 11.0)), 0, 11));
    const double low_mid = (bands[1] + bands[2] + bands[3]) / 3.0;
    const double upper = (bands[6] + bands[7] + bands[8] + bands[9]) / 4.0;
    return std::string{kCircleOfFifths[pitch_index]} + (low_mid > upper * 1.15 ? "m" : "");
}

double foldedBpm(double bpm) noexcept
{
    while (bpm > 180.0) {
        bpm *= 0.5;
    }
    while (bpm > 0.0 && bpm < 60.0) {
        bpm *= 2.0;
    }
    return bpm;
}

double median(std::vector<double> values)
{
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}

std::vector<double> recentValues(const std::vector<double>& values, std::size_t count)
{
    if (values.size() <= count) {
        return values;
    }
    return {values.end() - static_cast<std::ptrdiff_t>(count), values.end()};
}

std::vector<std::string> timelineSections(double elapsed_seconds, int duration_seconds)
{
    std::vector<std::string> sections{};
    if (duration_seconds <= 0) {
        if (elapsed_seconds >= 0.0) {
            sections.push_back("00:00 live section");
        }
        return sections;
    }
    const int duration = std::max(1, duration_seconds);
    const int marks[] = {0, duration / 4, duration / 2, (duration * 3) / 4};
    const char* names[] = {"intro", "section A", "section B", "outro"};
    for (std::size_t index = 0; index < 4; ++index) {
        const int mark = marks[index];
        const int minutes = mark / 60;
        const int seconds = mark % 60;
        std::string label = (minutes < 10 ? "0" : "") + std::to_string(minutes)
            + ":" + (seconds < 10 ? "0" : "") + std::to_string(seconds)
            + " " + names[index];
        if (elapsed_seconds >= static_cast<double>(mark)) {
            label += " *";
        }
        sections.push_back(std::move(label));
    }
    return sections;
}

} // namespace

void CreatorAnalysisRuntime::update(const application::AppServiceRegistry& services, double delta_seconds)
{
    const auto playback = services.playbackStatus().snapshot();
    const auto& session = services.playbackStatus().session();
    const std::string identity = playback.current_track_id ? std::to_string(*playback.current_track_id) : playback.title + "|" + playback.source_label;
    const std::string current_identity = track_id_ ? std::to_string(*track_id_) : title_ + "|" + source_label_;
    if (identity != current_identity) {
        resetForTrack(playback.current_track_id, playback.title, playback.source_label);
    }

    current_.available = playback.status != app::PlaybackStatus::Empty;
    current_.stem_status = "not-separated; source mix only";
    current_.analysis_source = "runtime visualization";
    current_.confidence = has_live_frame_ ? "estimated-live" : "waiting-for-audio";
    elapsed_seconds_ = playback.elapsed_seconds;

    if (playback.status == app::PlaybackStatus::Empty) {
        current_.status_message = "Creator analysis waiting for a loaded track";
        return;
    }
    if (!session.visualization_frame.available) {
        current_.status_message = "Creator analysis waiting for visualization frames";
        rebuildDerivedFields(playback.duration_seconds);
        return;
    }

    has_live_frame_ = true;
    const auto& bands = session.visualization_frame.bands;
    const double energy = bandsEnergy(bands);
    const double peak = bandsPeak(bands);
    const double rms = std::sqrt(std::inner_product(
        bands.begin(),
        bands.end(),
        bands.begin(),
        0.0,
        std::plus<>(),
        [](float left, float right) {
            return static_cast<double>(left) * static_cast<double>(right);
        }) / static_cast<double>(bands.size()));

    smoothed_energy_ = frame_count_ == 0 ? energy : (smoothed_energy_ * 0.88) + (energy * 0.12);
    peak_energy_ = std::max(peak_energy_ * 0.995, peak);
    rms_accumulator_ += rms * rms;
    energy_accumulator_ += energy;
    ++frame_count_;
    seconds_since_last_peak_ += std::max(0.0, delta_seconds);

    waveform_points_.push_back(static_cast<float>(std::clamp((energy * 2.0) - 1.0, -1.0, 1.0)));
    if (waveform_points_.size() > 64) {
        waveform_points_.erase(waveform_points_.begin(), waveform_points_.begin() + static_cast<std::ptrdiff_t>(waveform_points_.size() - 64));
    }

    recordBeat(playback.elapsed_seconds, energy, smoothed_energy_);
    current_.key = estimateKey(bands);
    const double running_rms = std::sqrt(rms_accumulator_ / static_cast<double>(std::max(1, frame_count_)));
    current_.lufs = -60.0 + (60.0 * clamp01(running_rms));
    current_.dynamic_range = std::clamp(20.0 * std::log10((peak_energy_ + 0.0001) / (running_rms + 0.0001)), 0.0, 60.0);
    current_.waveform_available = !waveform_points_.empty();
    current_.waveform_points = waveform_points_;
    rebuildDerivedFields(playback.duration_seconds);
    current_.status_message = "Creator analysis running from live runtime audio projection";
}

CreatorRuntimeSnapshot CreatorAnalysisRuntime::snapshot(std::uint64_t version) const
{
    auto result = current_;
    result.version = version;
    return result;
}

void CreatorAnalysisRuntime::reset()
{
    resetForTrack({}, {}, {});
}

void CreatorAnalysisRuntime::resetForTrack(std::optional<int> track_id, std::string title, std::string source_label)
{
    track_id_ = track_id;
    title_ = std::move(title);
    source_label_ = std::move(source_label);
    has_live_frame_ = false;
    elapsed_seconds_ = 0.0;
    smoothed_energy_ = 0.0;
    peak_energy_ = 0.0;
    rms_accumulator_ = 0.0;
    energy_accumulator_ = 0.0;
    frame_count_ = 0;
    seconds_since_last_peak_ = 999.0;
    waveform_points_.clear();
    beat_times_.clear();
    section_markers_.clear();
    current_ = {};
    current_.status_message = "Creator analysis waiting for runtime audio projection";
}

void CreatorAnalysisRuntime::recordBeat(double elapsed_seconds, double energy, double smoothed_energy)
{
    const bool strong_transient = energy > std::max(0.28, smoothed_energy * 1.18);
    if (!strong_transient || seconds_since_last_peak_ < 0.24) {
        return;
    }
    beat_times_.push_back(elapsed_seconds);
    seconds_since_last_peak_ = 0.0;
    if (beat_times_.size() > 64) {
        beat_times_.erase(beat_times_.begin(), beat_times_.begin() + static_cast<std::ptrdiff_t>(beat_times_.size() - 64));
    }

    std::vector<double> intervals{};
    for (std::size_t index = 1; index < beat_times_.size(); ++index) {
        const double interval = beat_times_[index] - beat_times_[index - 1];
        if (interval >= 0.25 && interval <= 2.0) {
            intervals.push_back(interval);
        }
    }
    if (!intervals.empty()) {
        current_.bpm = foldedBpm(60.0 / std::max(0.001, median(intervals)));
    }
}

void CreatorAnalysisRuntime::rebuildDerivedFields(int duration_seconds)
{
    current_.beat_grid_available = beat_times_.size() >= 2;
    current_.beat_grid_seconds = recentValues(beat_times_, 16);
    current_.loop_marker_seconds.clear();
    if (beat_times_.size() >= 5) {
        current_.loop_marker_seconds.push_back(beat_times_[beat_times_.size() - 5]);
        current_.loop_marker_seconds.push_back(beat_times_.back());
    }
    current_.section_markers = timelineSections(elapsed_seconds_, duration_seconds);
}

} // namespace lofibox::runtime
