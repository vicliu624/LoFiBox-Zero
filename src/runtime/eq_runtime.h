// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

#include "app/app_state.h"
#include "application/app_service_registry.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class EqRuntime {
public:
    EqRuntime(application::AppServiceRegistry services, app::EqState& eq) noexcept;

    void setEnabled(bool enabled);
    [[nodiscard]] bool setBand(int band_index, int gain_db);
    [[nodiscard]] bool adjustBand(int band_index, int delta_db);
    [[nodiscard]] bool applyPreset(std::string_view preset_name);
    [[nodiscard]] bool cyclePreset(int delta);
    void reset();
    [[nodiscard]] EqRuntimeSnapshot snapshot(std::uint64_t version) const;

private:
    void applyToPlayback() const;

    application::AppServiceRegistry services_;
    app::EqState& eq_;
};

} // namespace lofibox::runtime
