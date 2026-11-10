// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <stop_token>
#include <utility>

namespace lofibox::core {

// An explicit scope carries cancellation through existing synchronous provider
// calls on one background worker. It never changes another thread's operation.
inline thread_local std::stop_token current_operation_token{};

class OperationCancellationScope {
public:
    explicit OperationCancellationScope(std::stop_token token)
        : previous_(std::exchange(current_operation_token, std::move(token))) {}
    ~OperationCancellationScope() { current_operation_token = std::move(previous_); }
    OperationCancellationScope(const OperationCancellationScope&) = delete;
    OperationCancellationScope& operator=(const OperationCancellationScope&) = delete;
private:
    std::stop_token previous_;
};

inline bool operationCancelled() noexcept { return current_operation_token.stop_requested(); }

} // namespace lofibox::core
