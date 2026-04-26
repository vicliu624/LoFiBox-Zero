// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>
#include <string_view>

namespace lofibox::platform::host {

class SingleInstanceLock {
public:
    [[nodiscard]] static SingleInstanceLock acquire(std::string_view instance_name = "lofibox-zero");

    SingleInstanceLock();
    ~SingleInstanceLock();

    SingleInstanceLock(const SingleInstanceLock&) = delete;
    SingleInstanceLock& operator=(const SingleInstanceLock&) = delete;

    SingleInstanceLock(SingleInstanceLock&&) noexcept;
    SingleInstanceLock& operator=(SingleInstanceLock&&) noexcept;

    [[nodiscard]] bool acquired() const;
    [[nodiscard]] const std::string& message() const;

private:
    struct Impl;
    explicit SingleInstanceLock(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_{};
};

} // namespace lofibox::platform::host
