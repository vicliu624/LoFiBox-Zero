// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/runtime_result.h"

#include <utility>

namespace lofibox::runtime {

RuntimeCommandResult RuntimeCommandResult::reject(
    std::string code,
    std::string message,
    CommandOrigin origin,
    std::string correlation_id,
    std::uint64_t version_before_apply,
    std::uint64_t version_after_apply) noexcept
{
    return {
        false,
        false,
        std::move(code),
        std::move(message),
        origin,
        std::move(correlation_id),
        version_before_apply,
        version_after_apply};
}

RuntimeCommandResult RuntimeCommandResult::ok(
    std::string code,
    std::string message,
    CommandOrigin origin,
    std::string correlation_id,
    std::uint64_t version_before_apply,
    std::uint64_t version_after_apply,
    bool applied) noexcept
{
    return {
        true,
        applied,
        std::move(code),
        std::move(message),
        origin,
        std::move(correlation_id),
        version_before_apply,
        version_after_apply};
}

} // namespace lofibox::runtime
