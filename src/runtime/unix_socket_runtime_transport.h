// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include "runtime/runtime_command_client.h"
#include "runtime/runtime_command_server.h"
#include "runtime/runtime_event.h"
#include "runtime/runtime_transport.h"

namespace lofibox::runtime {

[[nodiscard]] std::filesystem::path defaultRuntimeSocketPath();

class UnixSocketRuntimeTransport final : public RuntimeTransport {
public:
    explicit UnixSocketRuntimeTransport(std::filesystem::path socket_path = defaultRuntimeSocketPath());

    [[nodiscard]] RuntimeCommandResponse sendCommand(const RuntimeCommandRequest& request) override;
    [[nodiscard]] RuntimeQueryResponse sendQuery(const RuntimeQueryRequest& request) override;

    [[nodiscard]] const std::filesystem::path& socketPath() const noexcept;
    [[nodiscard]] const std::string& lastError() const noexcept;

private:
    [[nodiscard]] std::string transact(std::string request_json);

    std::filesystem::path socket_path_{};
    std::string last_error_{};
};

class UnixSocketRuntimeCommandClient final : public RuntimeCommandClient {
public:
    explicit UnixSocketRuntimeCommandClient(std::filesystem::path socket_path = defaultRuntimeSocketPath());

    [[nodiscard]] RuntimeCommandResult dispatch(const RuntimeCommand& command) override;
    [[nodiscard]] RuntimeSnapshot query(const RuntimeQuery& query) const override;

    [[nodiscard]] const std::filesystem::path& socketPath() const noexcept;
    [[nodiscard]] const std::string& lastError() const noexcept;

private:
    mutable UnixSocketRuntimeTransport transport_;
};

class UnixSocketRuntimeEventStream {
public:
    explicit UnixSocketRuntimeEventStream(std::filesystem::path socket_path = defaultRuntimeSocketPath());
    ~UnixSocketRuntimeEventStream();

    UnixSocketRuntimeEventStream(const UnixSocketRuntimeEventStream&) = delete;
    UnixSocketRuntimeEventStream& operator=(const UnixSocketRuntimeEventStream&) = delete;

    [[nodiscard]] bool connect(RuntimeEventStreamRequest request = {});
    void close() noexcept;
    [[nodiscard]] std::optional<RuntimeEvent> next(std::chrono::milliseconds timeout);

    [[nodiscard]] const std::filesystem::path& socketPath() const noexcept;
    [[nodiscard]] const std::string& lastError() const noexcept;

private:
    std::filesystem::path socket_path_{};
    std::string last_error_{};
    int fd_{-1};
};

class UnixSocketRuntimeCommandServer {
public:
    explicit UnixSocketRuntimeCommandServer(RuntimeCommandServer& server, std::filesystem::path socket_path = defaultRuntimeSocketPath());
    ~UnixSocketRuntimeCommandServer();

    UnixSocketRuntimeCommandServer(const UnixSocketRuntimeCommandServer&) = delete;
    UnixSocketRuntimeCommandServer& operator=(const UnixSocketRuntimeCommandServer&) = delete;

    [[nodiscard]] bool start();
    void stop() noexcept;

    [[nodiscard]] const std::filesystem::path& socketPath() const noexcept;
    [[nodiscard]] const std::string& lastError() const noexcept;

private:
    void run();
    void handleClient(int client_fd) noexcept;
    void handleEventStream(int client_fd, RuntimeEventStreamRequest request) noexcept;

    RuntimeCommandServer& server_;
    std::filesystem::path socket_path_{};
    std::string last_error_{};
    std::atomic_bool running_{false};
    int server_fd_{-1};
    std::thread thread_{};
    std::mutex client_threads_mutex_{};
    std::vector<std::thread> client_threads_{};
};

} // namespace lofibox::runtime
