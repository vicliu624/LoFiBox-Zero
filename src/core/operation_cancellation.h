// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <stop_token>
#include <functional>
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

// Optional, nested policy checks on a background operation. Local/cache reads
// remain available; process capture loops use this to stop network work promptly.
class OperationCancellationGuard {
public:
    explicit OperationCancellationGuard(std::function<bool()> cancelled)
        : cancelled_(std::move(cancelled)), previous_(current_) { current_ = this; }
    ~OperationCancellationGuard() { current_ = previous_; }
    OperationCancellationGuard(const OperationCancellationGuard&) = delete;
    OperationCancellationGuard& operator=(const OperationCancellationGuard&) = delete;
    static bool active() noexcept { return current_ != nullptr; }
    static bool cancelled() noexcept
    {
        for (auto* guard = current_; guard; guard = guard->previous_) {
            try { if (guard->cancelled_()) return true; }
            catch (...) { return true; }
        }
        return false;
    }
private:
    std::function<bool()> cancelled_;
    const OperationCancellationGuard* previous_;
    inline static thread_local const OperationCancellationGuard* current_{};
};

inline bool operationCancelled() noexcept
{
    return current_operation_token.stop_requested() || OperationCancellationGuard::cancelled();
}

} // namespace lofibox::core
