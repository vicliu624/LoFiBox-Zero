// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/runtime_query_dispatcher.h"

namespace lofibox::runtime {

RuntimeQueryDispatcher::RuntimeQueryDispatcher(RuntimeSessionFacade& session) noexcept
    : session_(session)
{
}

RuntimeSnapshot RuntimeQueryDispatcher::query(const RuntimeQuery& query, std::uint64_t version) const
{
    switch (query.kind) {
    case RuntimeQueryKind::PlaybackSnapshot:
        {
            RuntimeSnapshot result{};
            result.version = version;
            result.playback = session_.playbackSnapshot(version);
            return result;
        }
    case RuntimeQueryKind::QueueSnapshot:
        {
            RuntimeSnapshot result{};
            result.version = version;
            result.queue = session_.queueSnapshot(version);
            return result;
        }
    case RuntimeQueryKind::EqSnapshot:
        {
            RuntimeSnapshot result{};
            result.version = version;
            result.eq = session_.eqSnapshot(version);
            return result;
        }
    case RuntimeQueryKind::RemoteSessionSnapshot:
        {
            RuntimeSnapshot result{};
            result.version = version;
            result.remote = session_.remoteSessionSnapshot(version);
            return result;
        }
    case RuntimeQueryKind::FullSnapshot:
        return session_.snapshot(version);
    }
    return session_.snapshot(version);
}

} // namespace lofibox::runtime
