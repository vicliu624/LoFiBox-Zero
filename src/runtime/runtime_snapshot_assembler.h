// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

#include "runtime/eq_runtime.h"
#include "runtime/playback_runtime.h"
#include "runtime/queue_runtime.h"
#include "runtime/remote_session_runtime.h"
#include "runtime/settings_runtime.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class RuntimeSnapshotAssembler {
public:
    [[nodiscard]] RuntimeSnapshot assemble(
        const PlaybackRuntime& playback,
        const QueueRuntime& queue,
        const EqRuntime& eq,
        const RemoteSessionRuntime& remote,
        const SettingsRuntime& settings,
        std::uint64_t version) const;
};

} // namespace lofibox::runtime
