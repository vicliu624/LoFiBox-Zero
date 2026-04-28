// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "runtime/runtime_command.h"
#include "runtime/runtime_command_dispatcher.h"
#include "runtime/runtime_query_dispatcher.h"
#include "runtime/runtime_result.h"
#include "runtime/runtime_session_facade.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class RuntimeCommandBus {
public:
    explicit RuntimeCommandBus(RuntimeSessionFacade& session) noexcept;

    [[nodiscard]] RuntimeCommandResult dispatch(const RuntimeCommand& command);
    [[nodiscard]] RuntimeSnapshot query(const RuntimeQuery& query) const;
    [[nodiscard]] RuntimeSnapshot snapshot() const;
    [[nodiscard]] std::uint64_t version() const noexcept;

private:
    RuntimeCommandDispatcher commands_;
    RuntimeQueryDispatcher queries_;
};

} // namespace lofibox::runtime
