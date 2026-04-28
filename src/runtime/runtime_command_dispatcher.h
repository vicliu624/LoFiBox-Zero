// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

#include "runtime/runtime_command.h"
#include "runtime/runtime_result.h"
#include "runtime/runtime_session_facade.h"

namespace lofibox::runtime {

class RuntimeCommandDispatcher {
public:
    explicit RuntimeCommandDispatcher(RuntimeSessionFacade& session) noexcept;

    [[nodiscard]] RuntimeCommandResult dispatch(const RuntimeCommand& command);
    [[nodiscard]] std::uint64_t version() const noexcept;

private:
    [[nodiscard]] RuntimeCommandResult rejectUnsupported(const RuntimeCommand& command, const char* message) const;
    [[nodiscard]] RuntimeCommandResult applied(const RuntimeCommand& command, const char* code, const char* message, bool applied);

    RuntimeSessionFacade& session_;
    std::uint64_t version_{0};
};

} // namespace lofibox::runtime
