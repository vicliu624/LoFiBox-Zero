// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/runtime_command_server.h"

#include "runtime/runtime_command_bus.h"

namespace lofibox::runtime {

RuntimeCommandServer::RuntimeCommandServer(RuntimeCommandBus& bus) noexcept
    : bus_(bus)
{
}

RuntimeCommandResult RuntimeCommandServer::dispatch(const RuntimeCommand& command)
{
    return bus_.dispatch(command);
}

RuntimeSnapshot RuntimeCommandServer::query(const RuntimeQuery& query) const
{
    return bus_.query(query);
}

RuntimeSnapshot RuntimeCommandServer::snapshot() const
{
    return bus_.snapshot();
}

} // namespace lofibox::runtime
