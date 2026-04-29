// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/remote_session_runtime.h"

#include <utility>

namespace lofibox::runtime {

void RemoteSessionRuntime::setSnapshot(RemoteSessionSnapshot snapshot)
{
    snapshot_ = std::move(snapshot);
}

void RemoteSessionRuntime::clearSnapshot() noexcept
{
    snapshot_ = {};
}

bool RemoteSessionRuntime::reconnect() const
{
    return snapshot_.stream_resolved;
}

RemoteSessionSnapshot RemoteSessionRuntime::snapshot(std::uint64_t version) const
{
    auto result = snapshot_;
    result.version = version;
    return result;
}

} // namespace lofibox::runtime
