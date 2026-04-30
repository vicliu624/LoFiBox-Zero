// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/runtime_command_bus.h"

namespace lofibox::runtime {

RuntimeCommandBus::RuntimeCommandBus(RuntimeSessionFacade& session) noexcept
    : session_(session),
      commands_(session),
      queries_(session)
{
}

RuntimeCommandResult RuntimeCommandBus::dispatch(const RuntimeCommand& command)
{
    const std::lock_guard lock{mutex_};
    return commands_.dispatch(command);
}

RuntimeSnapshot RuntimeCommandBus::query(const RuntimeQuery& query) const
{
    const std::lock_guard lock{mutex_};
    return queries_.query(query, commands_.version());
}

RuntimeSnapshot RuntimeCommandBus::snapshot() const
{
    return query(RuntimeQuery{RuntimeQueryKind::FullSnapshot});
}

void RuntimeCommandBus::tick(double delta_seconds)
{
    const std::lock_guard lock{mutex_};
    session_.tick(delta_seconds);
}

std::uint64_t RuntimeCommandBus::version() const noexcept
{
    const std::lock_guard lock{mutex_};
    return commands_.version();
}

} // namespace lofibox::runtime
