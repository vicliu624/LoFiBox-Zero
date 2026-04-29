// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include "app/app_state.h"
#include "application/app_service_registry.h"
#include "application/playback_command_service.h"
#include "runtime/eq_runtime.h"
#include "runtime/playback_runtime.h"
#include "runtime/queue_runtime.h"
#include "runtime/remote_session_runtime.h"
#include "runtime/runtime_snapshot_assembler.h"
#include "runtime/settings_runtime.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class RuntimeSessionFacade {
public:
    using RemoteTrackStarter = application::PlaybackCommandService::RemoteTrackStarter;
    using ActiveRemoteStreamStarter = std::function<bool()>;

    RuntimeSessionFacade(application::AppServiceRegistry services, app::EqState& eq) noexcept;

    void setRemoteTrackStarter(RemoteTrackStarter starter);
    void setActiveRemoteStreamStarter(ActiveRemoteStreamStarter starter);

    [[nodiscard]] PlaybackRuntime& playback() noexcept;
    [[nodiscard]] const PlaybackRuntime& playback() const noexcept;
    [[nodiscard]] QueueRuntime& queue() noexcept;
    [[nodiscard]] const QueueRuntime& queue() const noexcept;
    [[nodiscard]] EqRuntime& eq() noexcept;
    [[nodiscard]] const EqRuntime& eq() const noexcept;
    [[nodiscard]] RemoteSessionRuntime& remote() noexcept;
    [[nodiscard]] const RemoteSessionRuntime& remote() const noexcept;
    [[nodiscard]] SettingsRuntime& settings() noexcept;
    [[nodiscard]] const SettingsRuntime& settings() const noexcept;
    [[nodiscard]] RuntimeSnapshot snapshot(std::uint64_t version) const;

private:
    PlaybackRuntime playback_;
    QueueRuntime queue_;
    EqRuntime eq_;
    RemoteSessionRuntime remote_;
    SettingsRuntime settings_;
    RuntimeSnapshotAssembler snapshots_{};
};

} // namespace lofibox::runtime
