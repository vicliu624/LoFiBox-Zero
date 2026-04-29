// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/runtime_session_facade.h"

namespace lofibox::runtime {

RuntimeSessionFacade::RuntimeSessionFacade(application::AppServiceRegistry services, EqRuntimeState& eq, SettingsRuntimeState& settings) noexcept
    : services_(services),
      playback_(services_),
      queue_(services_),
      eq_(services_, eq),
      settings_(settings)
{
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

void RuntimeSessionFacade::tick(double delta_seconds)
{
    playback_.update(delta_seconds);
    creator_.update(services_, delta_seconds);
}

RuntimeSnapshot RuntimeSessionFacade::snapshot(std::uint64_t version) const
{
    return snapshots_.assemble(playback_, queue_, eq_, remote_, settings_, services_, creator_, version);
}

} // namespace lofibox::runtime
