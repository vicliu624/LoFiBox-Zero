// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/runtime_session_facade.h"

#include <utility>

namespace lofibox::runtime {

RuntimeSessionFacade::RuntimeSessionFacade(application::AppServiceRegistry services, app::EqState& eq) noexcept
    : playback_(services),
      queue_(services),
      eq_(services, eq)
{
}

void RuntimeSessionFacade::setRemoteTrackStarter(RemoteTrackStarter starter)
{
    playback_.setRemoteTrackStarter(starter);
    queue_.setRemoteTrackStarter(std::move(starter));
}

void RuntimeSessionFacade::setActiveRemoteStreamStarter(ActiveRemoteStreamStarter starter)
{
    remote_.setActiveRemoteStreamStarter(std::move(starter));
}

PlaybackRuntime& RuntimeSessionFacade::playback() noexcept
{
    return playback_;
}

const PlaybackRuntime& RuntimeSessionFacade::playback() const noexcept
{
    return playback_;
}

QueueRuntime& RuntimeSessionFacade::queue() noexcept
{
    return queue_;
}

const QueueRuntime& RuntimeSessionFacade::queue() const noexcept
{
    return queue_;
}

EqRuntime& RuntimeSessionFacade::eq() noexcept
{
    return eq_;
}

const EqRuntime& RuntimeSessionFacade::eq() const noexcept
{
    return eq_;
}

RemoteSessionRuntime& RuntimeSessionFacade::remote() noexcept
{
    return remote_;
}

const RemoteSessionRuntime& RuntimeSessionFacade::remote() const noexcept
{
    return remote_;
}

SettingsRuntime& RuntimeSessionFacade::settings() noexcept
{
    return settings_;
}

const SettingsRuntime& RuntimeSessionFacade::settings() const noexcept
{
    return settings_;
}

RuntimeSnapshot RuntimeSessionFacade::snapshot(std::uint64_t version) const
{
    return snapshots_.assemble(playback_, queue_, eq_, remote_, settings_, version);
}

} // namespace lofibox::runtime
