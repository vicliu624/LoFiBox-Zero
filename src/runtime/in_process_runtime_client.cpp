// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/in_process_runtime_client.h"

#include "runtime/runtime_command_server.h"

namespace lofibox::runtime {

InProcessRuntimeCommandClient::InProcessRuntimeCommandClient(RuntimeCommandServer& server) noexcept
    : server_(server)
{
}

RuntimeCommandResult InProcessRuntimeCommandClient::dispatch(const RuntimeCommand& command)
{
    return server_.dispatch(command);
}

RuntimeSnapshot InProcessRuntimeCommandClient::query(const RuntimeQuery& query) const
{
    return server_.query(query);
}

} // namespace lofibox::runtime
