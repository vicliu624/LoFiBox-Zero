// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/runtime_snapshot_assembler.h"

namespace lofibox::runtime {

RuntimeSnapshot RuntimeSnapshotAssembler::assemble(
    const PlaybackRuntime& playback,
    const QueueRuntime& queue,
    const EqRuntime& eq,
    const RemoteSessionRuntime& remote,
    const SettingsRuntime& settings,
    std::uint64_t version) const
{
    RuntimeSnapshot result{};
    result.version = version;
    result.playback = playback.snapshot(version);
    result.queue = queue.snapshot(version);
    result.eq = eq.snapshot(version);
    result.remote = remote.snapshot(version);
    result.settings = settings.snapshot(version);
    return result;
}

} // namespace lofibox::runtime
