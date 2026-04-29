// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class RemoteSessionRuntime {
public:
    using ActiveRemoteStreamStarter = std::function<bool()>;

    void setActiveRemoteStreamStarter(ActiveRemoteStreamStarter starter);
    void setSnapshot(RemoteSessionSnapshot snapshot);
    void clearSnapshot() noexcept;

    [[nodiscard]] bool startActiveStream() const;
    [[nodiscard]] RemoteSessionSnapshot snapshot(std::uint64_t version) const;

private:
    ActiveRemoteStreamStarter active_remote_stream_starter_{};
    RemoteSessionSnapshot snapshot_{};
};

} // namespace lofibox::runtime
