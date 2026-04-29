// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "application/app_service_registry.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class CreatorAnalysisRuntime {
public:
    void update(const application::AppServiceRegistry& services, double delta_seconds);
    [[nodiscard]] CreatorRuntimeSnapshot snapshot(std::uint64_t version) const;
    void reset();

private:
    void resetForTrack(std::optional<int> track_id, std::string title, std::string source_label);
    void recordBeat(double elapsed_seconds, double energy, double smoothed_energy);
    void rebuildDerivedFields(int duration_seconds);

    std::optional<int> track_id_{};
    std::string title_{};
    std::string source_label_{};
    bool has_live_frame_{false};
    double elapsed_seconds_{0.0};
    double smoothed_energy_{0.0};
    double peak_energy_{0.0};
    double rms_accumulator_{0.0};
    double energy_accumulator_{0.0};
    int frame_count_{0};
    double seconds_since_last_peak_{999.0};
    std::vector<float> waveform_points_{};
    std::vector<double> beat_times_{};
    std::vector<std::string> section_markers_{};
    CreatorRuntimeSnapshot current_{};
};

} // namespace lofibox::runtime
