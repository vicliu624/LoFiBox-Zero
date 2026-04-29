// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "application/app_service_registry.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class QueueRuntime {
public:
    explicit QueueRuntime(application::AppServiceRegistry services) noexcept;

    [[nodiscard]] bool step(int delta) const;
    [[nodiscard]] bool jump(int queue_index) const;
    void clear() const noexcept;
    void cycleMainMenuPlaybackMode() const;
    void toggleShuffle() const;
    void cycleRepeatMode() const noexcept;
    void setRepeatAll(bool enabled) const noexcept;
    void setRepeatOne(bool enabled) const noexcept;
    [[nodiscard]] QueueRuntimeSnapshot snapshot(std::uint64_t version) const;

private:
    application::AppServiceRegistry services_;
};

} // namespace lofibox::runtime
