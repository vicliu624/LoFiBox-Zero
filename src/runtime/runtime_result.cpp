// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/runtime_result.h"

#include <utility>

namespace lofibox::runtime {

RuntimeCommandResult RuntimeCommandResult::reject(
    std::string code,
    std::string message,
    std::string correlation_id,
    std::uint64_t version) noexcept
{
    return {false, false, std::move(code), std::move(message), std::move(correlation_id), version};
}

RuntimeCommandResult RuntimeCommandResult::ok(
    std::string code,
    std::string message,
    std::string correlation_id,
    std::uint64_t version,
    bool applied) noexcept
{
    return {true, applied, std::move(code), std::move(message), std::move(correlation_id), version};
}

} // namespace lofibox::runtime
