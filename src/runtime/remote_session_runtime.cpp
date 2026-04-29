// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/remote_session_runtime.h"

#include <utility>

namespace lofibox::runtime {

void RemoteSessionRuntime::setActiveRemoteStreamStarter(ActiveRemoteStreamStarter starter)
{
    active_remote_stream_starter_ = std::move(starter);
}

void RemoteSessionRuntime::setSnapshot(RemoteSessionSnapshot snapshot)
{
    snapshot_ = std::move(snapshot);
}

void RemoteSessionRuntime::clearSnapshot() noexcept
{
    snapshot_ = {};
}

bool RemoteSessionRuntime::startActiveStream() const
{
    return active_remote_stream_starter_ ? active_remote_stream_starter_() : false;
}

RemoteSessionSnapshot RemoteSessionRuntime::snapshot(std::uint64_t version) const
{
    auto result = snapshot_;
    result.version = version;
    return result;
}

} // namespace lofibox::runtime
