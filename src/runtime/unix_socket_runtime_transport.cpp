// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/unix_socket_runtime_transport.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <system_error>
#include <thread>
#include <utility>

#include "runtime/runtime_envelope_serializer.h"

#if defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace lofibox::runtime {
namespace {

std::filesystem::path runtimeRoot()
{
    if (const char* xdg = std::getenv("XDG_RUNTIME_DIR"); xdg != nullptr && *xdg != '\0') {
        return std::filesystem::path{xdg};
    }
    return std::filesystem::temp_directory_path();
}

std::array<unsigned char, 4> encodeLength(std::uint32_t value) noexcept
{
    return {
        static_cast<unsigned char>((value >> 24U) & 0xffU),
        static_cast<unsigned char>((value >> 16U) & 0xffU),
        static_cast<unsigned char>((value >> 8U) & 0xffU),
        static_cast<unsigned char>(value & 0xffU)};
}

std::uint32_t decodeLength(const std::array<unsigned char, 4>& bytes) noexcept
{
    return (static_cast<std::uint32_t>(bytes[0]) << 24U)
        | (static_cast<std::uint32_t>(bytes[1]) << 16U)
        | (static_cast<std::uint32_t>(bytes[2]) << 8U)
        | static_cast<std::uint32_t>(bytes[3]);
}

#if defined(__unix__) || defined(__APPLE__)
bool writeAll(int fd, const void* data, std::size_t size) noexcept
{
    const auto* bytes = static_cast<const unsigned char*>(data);
    std::size_t written = 0;
    while (written < size) {
#if defined(MSG_NOSIGNAL)
        constexpr int kSendFlags = MSG_NOSIGNAL;
#else
        constexpr int kSendFlags = 0;
#endif
        const auto count = ::send(fd, bytes + written, size - written, kSendFlags);
        if (count <= 0) {
            return false;
        }
        written += static_cast<std::size_t>(count);
    }
    return true;
}

bool readAll(int fd, void* data, std::size_t size) noexcept
{
    auto* bytes = static_cast<unsigned char*>(data);
    std::size_t read = 0;
    while (read < size) {
        const auto count = ::recv(fd, bytes + read, size - read, 0);
        if (count <= 0) {
            return false;
        }
        read += static_cast<std::size_t>(count);
    }
    return true;
}

bool writeFrame(int fd, const std::string& json) noexcept
{
    if (json.size() > static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        return false;
    }
    const auto length = encodeLength(static_cast<std::uint32_t>(json.size()));
    return writeAll(fd, length.data(), length.size()) && writeAll(fd, json.data(), json.size());
}

std::string readFrame(int fd)
{
    std::array<unsigned char, 4> length_bytes{};
    if (!readAll(fd, length_bytes.data(), length_bytes.size())) {
        return {};
    }
    const auto length = decodeLength(length_bytes);
    if (length == 0 || length > (1024U * 1024U)) {
        return {};
    }
    std::string json(length, '\0');
    if (!readAll(fd, json.data(), json.size())) {
        return {};
    }
    return json;
}

void closeFd(int& fd) noexcept
{
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

struct SocketFdOwner {
    int& fd;
    ~SocketFdOwner() { closeFd(fd); }
};

int openUnixSocket() noexcept
{
#if defined(__linux__)
    return ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
#else
    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd >= 0 && ::fcntl(fd, F_SETFD, FD_CLOEXEC) != 0) closeFd(fd);
    return fd;
#endif
}

int acceptUnixSocket(int listener) noexcept
{
#if defined(__linux__)
    return ::accept4(listener, nullptr, nullptr, SOCK_CLOEXEC);
#else
    int fd = ::accept(listener, nullptr, nullptr);
    if (fd >= 0 && ::fcntl(fd, F_SETFD, FD_CLOEXEC) != 0) closeFd(fd);
    return fd;
#endif
}
#endif

RuntimeCommandResult transportReject(std::string message)
{
    return RuntimeCommandResult::reject(
        "RUNTIME_TRANSPORT_ERROR",
        std::move(message),
        CommandOrigin::RuntimeCli,
        {},
        0,
        0);
}

} // namespace

std::filesystem::path defaultRuntimeSocketPath()
{
    return runtimeRoot() / "lofibox" / "runtime.sock";
}

UnixSocketRuntimeTransport::UnixSocketRuntimeTransport(std::filesystem::path socket_path)
    : socket_path_(socket_path.empty() ? defaultRuntimeSocketPath() : std::move(socket_path))
{
}

RuntimeCommandResponse UnixSocketRuntimeTransport::sendCommand(const RuntimeCommandRequest& request)
{
    auto response = parseRuntimeCommandResponse(transact(serializeRuntimeCommandRequest(request)));
    if (response) {
        return *response;
    }
    return {transportReject(last_error_.empty() ? "Invalid runtime command response." : last_error_)};
}

RuntimeQueryResponse UnixSocketRuntimeTransport::sendQuery(const RuntimeQueryRequest& request)
{
    auto response = parseRuntimeQueryResponse(transact(serializeRuntimeQueryRequest(request)));
    if (response) {
        return *response;
    }
    return {};
}

const std::filesystem::path& UnixSocketRuntimeTransport::socketPath() const noexcept
{
    return socket_path_;
}

const std::string& UnixSocketRuntimeTransport::lastError() const noexcept
{
    return last_error_;
}

std::string UnixSocketRuntimeTransport::transact(std::string request_json)
{
    last_error_.clear();
#if defined(__unix__) || defined(__APPLE__)
    int fd = openUnixSocket();
    const SocketFdOwner connection{fd};
    if (fd < 0) {
        last_error_ = "Could not create runtime socket.";
        return {};
    }

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    const auto path = socket_path_.string();
    if (path.size() >= sizeof(address.sun_path)) {
        last_error_ = "Runtime socket path is too long.";
        closeFd(fd);
        return {};
    }
    std::strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);
    if (::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        last_error_ = "Runtime is not reachable at " + path + ".";
        closeFd(fd);
        return {};
    }
    if (!writeFrame(fd, request_json)) {
        last_error_ = "Could not write runtime request.";
        closeFd(fd);
        return {};
    }
    auto response = readFrame(fd);
    closeFd(fd);
    if (response.empty()) {
        last_error_ = "Could not read runtime response.";
    }
    return response;
#else
    (void)request_json;
    last_error_ = "Unix runtime transport is unavailable on this platform.";
    return {};
#endif
}

UnixSocketRuntimeCommandClient::UnixSocketRuntimeCommandClient(std::filesystem::path socket_path)
    : transport_(std::move(socket_path))
{
}

RuntimeCommandResult UnixSocketRuntimeCommandClient::dispatch(const RuntimeCommand& command)
{
    return transport_.sendCommand(RuntimeCommandRequest{command}).result;
}

RuntimeSnapshot UnixSocketRuntimeCommandClient::query(const RuntimeQuery& query) const
{
    return transport_.sendQuery(RuntimeQueryRequest{query}).snapshot;
}

const std::filesystem::path& UnixSocketRuntimeCommandClient::socketPath() const noexcept
{
    return transport_.socketPath();
}

const std::string& UnixSocketRuntimeCommandClient::lastError() const noexcept
{
    return transport_.lastError();
}

UnixSocketRuntimeEventStream::UnixSocketRuntimeEventStream(std::filesystem::path socket_path)
    : socket_path_(socket_path.empty() ? defaultRuntimeSocketPath() : std::move(socket_path))
{
}

UnixSocketRuntimeEventStream::~UnixSocketRuntimeEventStream()
{
    close();
}

bool UnixSocketRuntimeEventStream::connect(RuntimeEventStreamRequest request)
{
    close();
    last_error_.clear();
#if defined(__unix__) || defined(__APPLE__)
    fd_ = openUnixSocket();
    if (fd_ < 0) {
        last_error_ = "Could not create runtime event socket.";
        return false;
    }

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    const auto path = socket_path_.string();
    if (path.size() >= sizeof(address.sun_path)) {
        last_error_ = "Runtime socket path is too long.";
        close();
        return false;
    }
    std::strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);
    if (::connect(fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        last_error_ = "Runtime is not reachable at " + path + ".";
        close();
        return false;
    }
    if (!writeFrame(fd_, serializeRuntimeEventStreamRequest(request))) {
        last_error_ = "Could not write runtime event stream request.";
        close();
        return false;
    }
    return true;
#else
    (void)request;
    last_error_ = "Unix runtime event stream is unavailable on this platform.";
    return false;
#endif
}

void UnixSocketRuntimeEventStream::close() noexcept
{
#if defined(__unix__) || defined(__APPLE__)
    closeFd(fd_);
#endif
}

std::optional<RuntimeEvent> UnixSocketRuntimeEventStream::next(std::chrono::milliseconds timeout)
{
#if defined(__unix__) || defined(__APPLE__)
    if (fd_ < 0) {
        last_error_ = "Runtime event stream is not connected.";
        return std::nullopt;
    }
    pollfd poll_fd{};
    poll_fd.fd = fd_;
    poll_fd.events = POLLIN;
    const int ready = ::poll(&poll_fd, 1, static_cast<int>(timeout.count()));
    if (ready == 0) {
        return std::nullopt;
    }
    if (ready < 0 || (poll_fd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
        last_error_ = "Runtime event stream disconnected.";
        close();
        return std::nullopt;
    }
    if ((poll_fd.revents & POLLIN) == 0) {
        return std::nullopt;
    }
    auto frame = readFrame(fd_);
    if (frame.empty()) {
        last_error_ = "Could not read runtime event frame.";
        close();
        return std::nullopt;
    }
    auto event = parseRuntimeEvent(frame);
    if (!event) {
        last_error_ = "Runtime event frame could not be parsed.";
        return std::nullopt;
    }
    return event;
#else
    (void)timeout;
    return std::nullopt;
#endif
}

const std::filesystem::path& UnixSocketRuntimeEventStream::socketPath() const noexcept
{
    return socket_path_;
}

const std::string& UnixSocketRuntimeEventStream::lastError() const noexcept
{
    return last_error_;
}

struct UnixSocketRuntimeCommandServer::ClientWorker {
    std::mutex socket_mutex{};
    int fd{-1};
    std::atomic_bool finished{false};
    std::thread thread{};
};

UnixSocketRuntimeCommandServer::UnixSocketRuntimeCommandServer(RuntimeCommandServer& server, std::filesystem::path socket_path)
    : server_(server),
      socket_path_(socket_path.empty() ? defaultRuntimeSocketPath() : std::move(socket_path))
{
}

UnixSocketRuntimeCommandServer::~UnixSocketRuntimeCommandServer()
{
    stop();
}

bool UnixSocketRuntimeCommandServer::start()
{
    stop();
    last_error_.clear();
#if defined(__unix__) || defined(__APPLE__)
    std::error_code ec{};
    std::filesystem::create_directories(socket_path_.parent_path(), ec);
    std::filesystem::remove(socket_path_, ec);
    server_fd_ = openUnixSocket();
    if (server_fd_ < 0) {
        last_error_ = "Could not create runtime server socket.";
        return false;
    }

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    const auto path = socket_path_.string();
    if (path.size() >= sizeof(address.sun_path)) {
        last_error_ = "Runtime socket path is too long.";
        closeFd(server_fd_);
        return false;
    }
    std::strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);
    if (::bind(server_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
        last_error_ = "Could not bind runtime socket at " + path + ".";
        closeFd(server_fd_);
        return false;
    }
    if (::listen(server_fd_, 8) != 0) {
        last_error_ = "Could not listen on runtime socket.";
        closeFd(server_fd_);
        return false;
    }
    running_ = true;
    thread_ = std::thread([this]() { run(); });
    return true;
#else
    last_error_ = "Unix runtime transport is unavailable on this platform.";
    return false;
#endif
}

void UnixSocketRuntimeCommandServer::stop() noexcept
{
    running_ = false;
#if defined(__unix__) || defined(__APPLE__)
    if (server_fd_ >= 0) {
        int wake_fd = openUnixSocket();
        if (wake_fd >= 0) {
            sockaddr_un address{};
            address.sun_family = AF_UNIX;
            const auto path = socket_path_.string();
            if (path.size() < sizeof(address.sun_path)) {
                std::strncpy(address.sun_path, path.c_str(), sizeof(address.sun_path) - 1);
                (void)::connect(wake_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address));
            }
            ::close(wake_fd);
        }
        (void)::shutdown(server_fd_, SHUT_RDWR);
    }
#endif
    if (thread_.joinable()) {
        thread_.join();
    }
#if defined(__unix__) || defined(__APPLE__)
    closeFd(server_fd_);
    for (const auto& client : clients_) {
        const std::lock_guard lock{client->socket_mutex};
        if (client->fd >= 0) ::shutdown(client->fd, SHUT_RDWR);
    }
#endif
    for (const auto& client : clients_) if (client->thread.joinable()) client->thread.join();
    clients_.clear();
    std::error_code ec{};
    std::filesystem::remove(socket_path_, ec);
}

const std::filesystem::path& UnixSocketRuntimeCommandServer::socketPath() const noexcept
{
    return socket_path_;
}

const std::string& UnixSocketRuntimeCommandServer::lastError() const noexcept
{
    return last_error_;
}

void UnixSocketRuntimeCommandServer::run()
{
#if defined(__unix__) || defined(__APPLE__)
    while (running_) {
        const int client_fd = acceptUnixSocket(server_fd_);
        if (client_fd < 0) {
            if (running_) {
                last_error_ = "Runtime socket accept failed.";
            }
            continue;
        }
        std::erase_if(clients_, [](const auto& client) {
            if (!client->finished.load(std::memory_order_acquire)) return false;
            client->thread.join();
            return true;
        });
        if (!running_ || clients_.size() >= 16) {
            ::close(client_fd);
            continue;
        }
        clients_.push_back(std::make_unique<ClientWorker>());
        auto* client = clients_.back().get();
        client->fd = client_fd;
        try {
            client->thread = std::thread([this, client, client_fd]() {
                try { handleClient(client_fd); }
                catch (...) { /* A failed request still releases its connection. */ }
                {
                    const std::lock_guard lock{client->socket_mutex};
                    closeFd(client->fd);
                }
                client->finished.store(true, std::memory_order_release);
            });
        } catch (...) {
            ::close(client_fd);
            clients_.pop_back();
        }
    }
#endif
}

void UnixSocketRuntimeCommandServer::handleClient(int client_fd)
{
#if defined(__unix__) || defined(__APPLE__)
    auto request = readFrame(client_fd);
    if (request.empty()) {
        return;
    }

    std::string response;
    if (auto event_stream = parseRuntimeEventStreamRequest(request)) {
        handleEventStream(client_fd, *event_stream);
        return;
    }
    if (auto command = parseRuntimeCommandRequest(request)) {
        response = serializeRuntimeCommandResponse(RuntimeCommandResponse{server_.dispatch(command->command)});
    } else if (auto query = parseRuntimeQueryRequest(request)) {
        response = serializeRuntimeQueryResponse(RuntimeQueryResponse{server_.query(query->query)});
    } else {
        response = serializeRuntimeCommandResponse(RuntimeCommandResponse{RuntimeCommandResult::reject(
            "RUNTIME_BAD_REQUEST",
            "Runtime request envelope could not be parsed.",
            CommandOrigin::Unknown,
            {},
            0,
            0)});
    }
    (void)writeFrame(client_fd, response);
#else
    (void)client_fd;
#endif
}

void UnixSocketRuntimeCommandServer::handleEventStream(int client_fd, RuntimeEventStreamRequest request)
{
#if defined(__unix__) || defined(__APPLE__)
    std::optional<RuntimeSnapshot> previous{};
    std::uint64_t last_event_version = request.after_version;
    int sent_events = 0;
    const auto heartbeat = std::chrono::milliseconds{std::max(100, request.heartbeat_ms)};
    auto last_sent = std::chrono::steady_clock::now() - heartbeat;

    while (running_) {
        const auto snapshot = server_.query(request.query);
        auto events = runtimeEventsBetween(previous, snapshot, runtimeEventTimestampMs());
        const auto now = std::chrono::steady_clock::now();
        if (events.empty() && (now - last_sent) >= heartbeat) {
            RuntimeEvent heartbeat_event{};
            heartbeat_event.kind = RuntimeEventKind::RuntimeSnapshot;
            heartbeat_event.version = snapshot.version;
            heartbeat_event.timestamp_ms = runtimeEventTimestampMs();
            heartbeat_event.snapshot = snapshot;
            heartbeat_event.elapsed_seconds = snapshot.playback.elapsed_seconds;
            heartbeat_event.duration_seconds = snapshot.playback.duration_seconds;
            heartbeat_event.current_index = snapshot.lyrics.current_index;
            heartbeat_event.offset_seconds = snapshot.lyrics.offset_seconds;
            events.push_back(std::move(heartbeat_event));
        }

        for (auto& event : events) {
            if (event.version <= last_event_version) {
                event.version = ++last_event_version;
            } else {
                last_event_version = event.version;
            }
            if (!writeFrame(client_fd, serializeRuntimeEvent(event))) {
                closeFd(client_fd);
                return;
            }
            ++sent_events;
            last_sent = std::chrono::steady_clock::now();
            if (request.max_events > 0 && sent_events >= request.max_events) {
                closeFd(client_fd);
                return;
            }
        }

        previous = snapshot;
        std::this_thread::sleep_for(std::chrono::milliseconds{33});
    }
#else
    (void)client_fd;
    (void)request;
#endif
}

} // namespace lofibox::runtime
