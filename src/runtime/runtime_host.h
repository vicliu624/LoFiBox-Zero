// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <memory>
#include <string>

#include "application/app_service_registry.h"
#include "runtime/eq_runtime_state.h"
#include "runtime/in_process_runtime_client.h"
#include "runtime/remote_session_runtime.h"
#include "runtime/runtime_command_bus.h"
#include "runtime/runtime_command_server.h"
#include "runtime/runtime_session_facade.h"
#include "runtime/runtime_snapshot.h"
#include "runtime/settings_runtime_state.h"

namespace lofibox::runtime {

class UnixSocketRuntimeCommandServer;

class RuntimeHost {
public:
    explicit RuntimeHost(application::AppServiceRegistry services);
    ~RuntimeHost();

    RuntimeHost(const RuntimeHost&) = delete;
    RuntimeHost& operator=(const RuntimeHost&) = delete;
    RuntimeHost(RuntimeHost&&) = delete;
    RuntimeHost& operator=(RuntimeHost&&) = delete;

    [[nodiscard]] RuntimeCommandClient& client() noexcept;
    [[nodiscard]] RuntimeCommandServer& server() noexcept;
    [[nodiscard]] const RuntimeCommandServer& server() const noexcept;

    void setRemoteSessionSnapshot(RemoteSessionSnapshot snapshot);
    void tick(double delta_seconds);

    void resetEq();

    [[nodiscard]] bool startExternalTransport(const std::filesystem::path& socket_path = {});
    void stopExternalTransport() noexcept;
    [[nodiscard]] std::string externalTransportPath() const;
    [[nodiscard]] std::string externalTransportError() const;

    [[nodiscard]] bool shutdownRequested() const noexcept;
    [[nodiscard]] bool reloadRequested() const noexcept;

private:
    EqRuntimeState eq_state_{};
    SettingsRuntimeState settings_state_{};
    RuntimeSessionFacade session_;
    RuntimeCommandBus bus_;
    RuntimeCommandServer server_;
    InProcessRuntimeCommandClient client_;
    std::unique_ptr<UnixSocketRuntimeCommandServer> external_server_{};
};

} // namespace lofibox::runtime
