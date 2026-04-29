// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "runtime/runtime_command.h"
#include "runtime/runtime_result.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

struct RuntimeCommandRequest {
    RuntimeCommand command{};
};

struct RuntimeCommandResponse {
    RuntimeCommandResult result{};
};

struct RuntimeQueryRequest {
    RuntimeQuery query{};
};

struct RuntimeQueryResponse {
    RuntimeSnapshot snapshot{};
};

class RuntimeTransport {
public:
    virtual ~RuntimeTransport() = default;

    [[nodiscard]] virtual RuntimeCommandResponse sendCommand(const RuntimeCommandRequest& request) = 0;
    [[nodiscard]] virtual RuntimeQueryResponse sendQuery(const RuntimeQueryRequest& request) = 0;
};

} // namespace lofibox::runtime
