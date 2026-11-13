// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/runtime_host.h"

#include <utility>

#include "runtime/runtime_command.h"
#include "runtime/unix_socket_runtime_transport.h"

namespace lofibox::runtime {

RuntimeHost::RuntimeHost(application::AppServiceRegistry services)
    : session_(services, eq_state_, settings_state_),
      bus_(session_),
      server_(bus_),
      client_(server_)
{
}

RuntimeHost::~RuntimeHost()
{
    stopExternalTransport();
}

RuntimeCommandClient& RuntimeHost::client() noexcept
{
    return client_;
}

RuntimeCommandServer& RuntimeHost::server() noexcept
{
    return server_;
}

const RuntimeCommandServer& RuntimeHost::server() const noexcept
{
    return server_;
}

void RuntimeHost::setRemoteSessionSnapshot(RemoteSessionSnapshot snapshot)
{
    const std::lock_guard lock{session_.stateMutex()};
    session_.remote().setSnapshot(std::move(snapshot));
}

void RuntimeHost::tick(double delta_seconds)
{
    bus_.tick(delta_seconds);
}

void RuntimeHost::resetEq()
{
    const std::lock_guard lock{session_.stateMutex()};
    session_.eq().reset();
}

bool RuntimeHost::startExternalTransport(const std::filesystem::path& socket_path)
{
    const std::lock_guard lock{external_mutex_};
    stopExternalTransport();
    external_server_ = std::make_unique<UnixSocketRuntimeCommandServer>(server_, socket_path);
    if (external_server_->start()) {
        return true;
    }
    return false;
}

void RuntimeHost::stopExternalTransport() noexcept
{
    const std::lock_guard lock{external_mutex_};
    if (external_server_) {
        external_server_->stop();
        external_server_.reset();
    }
}

std::string RuntimeHost::externalTransportPath() const
{
    const std::lock_guard lock{external_mutex_};
    return external_server_ ? external_server_->socketPath().string() : std::string{};
}

std::string RuntimeHost::externalTransportError() const
{
    const std::lock_guard lock{external_mutex_};
    return external_server_ ? external_server_->lastError() : std::string{};
}

bool RuntimeHost::shutdownRequested() const noexcept
{
    const std::lock_guard lock{session_.stateMutex()};
    return session_.settings().shutdownRequested();
}

std::chrono::seconds RuntimeHost::screenTimeout() const noexcept
{
    const std::lock_guard lock{session_.stateMutex()};
    return session_.settings().screenTimeout();
}

bool RuntimeHost::reloadRequested() const noexcept
{
    const std::lock_guard lock{session_.stateMutex()};
    return session_.settings().reloadRequested();
}

} // namespace lofibox::runtime
