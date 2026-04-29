// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class RemoteSessionRuntime {
public:
    void setSnapshot(RemoteSessionSnapshot snapshot);
    void clearSnapshot() noexcept;

    [[nodiscard]] bool reconnect() const;
    [[nodiscard]] RemoteSessionSnapshot snapshot(std::uint64_t version) const;

private:
    RemoteSessionSnapshot snapshot_{};
};

} // namespace lofibox::runtime
