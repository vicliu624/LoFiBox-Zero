// SPDX-License-Identifier: GPL-3.0-or-later

#include "webui/webui_server.h"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <mutex>
#include <optional>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <stop_token>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "webui/webui_http_router.h"
#include "webui/webui_json.h"
#include "webui/webui_runtime_adapter.h"
#include "webui/webui_ws_runtime_stream.h"
#include "webui/webui_socket_io.h"
#include "core/operation_cancellation.h"

namespace lofibox::webui {
namespace {

constexpr int kListenBacklog = 4;
constexpr std::size_t kReadBufferSize = 4096;

// Parse the HTTP method from the request line (e.g. "GET /path HTTP/1.1").
std::string_view parseMethod(std::string_view request_line)
{
    auto space = request_line.find(' ');
    if (space == std::string_view::npos) return {};
    return request_line.substr(0, space);
}

// Parse the URL path from the request line.
std::string_view parsePath(std::string_view request_line)
{
    auto start = request_line.find(' ');
    if (start == std::string_view::npos) return {};
    ++start;
    auto end = request_line.find(' ', start);
    if (end == std::string_view::npos) return {};
    return request_line.substr(start, end - start);
}

// Check if the request is a WebSocket upgrade.
std::string_view trimHeader(std::string_view value)
{
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) value.remove_prefix(1);
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t')) value.remove_suffix(1);
    return value;
}

bool sameHeader(std::string_view a, std::string_view b)
{
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
        if (lower(a[i]) != lower(b[i])) return false;
    }
    return true;
}

bool isWebSocketUpgrade(std::string_view headers)
{
    std::size_t position = headers.find("\r\n") + 2;
    while (position < headers.size()) {
        const auto end = headers.find("\r\n", position);
        if (end == std::string_view::npos || end == position) break;
        const auto line = headers.substr(position, end - position);
        const auto colon = line.find(':');
        if (colon != std::string_view::npos && sameHeader(line.substr(0, colon), "Upgrade")) {
            return sameHeader(trimHeader(line.substr(colon + 1)), "websocket");
        }
        position = end + 2;
    }
    return false;
}

} // namespace

struct WebUiServer::Worker {
    int fd{-1};
    std::mutex socket_mutex;
    std::thread thread;
    std::atomic_bool finished{false};
    std::stop_source cancellation{};
};

WebUiServer::WebUiServer(WebUiConfig config, WebUiRuntimeAdapter& adapter)
    : config_(std::move(config))
    , adapter_(adapter)
    , router_(std::make_unique<WebUiHttpRouter>(adapter_))
{
}

WebUiServer::~WebUiServer()
{
    stop();
}

void WebUiServer::setLibraryQueryProvider(app::LibraryQueryProvider* provider) noexcept
{
    if (router_) {
        router_->setLibraryQueryProvider(provider);
    }
}

void WebUiServer::setLibraryEnrichProvider(app::LibraryEnrichProvider* provider) noexcept
{
    if (router_) {
        router_->setLibraryEnrichProvider(provider);
    }
}

void WebUiServer::setTheme(const ui::UiTheme* theme) noexcept
{
    if (router_) {
        router_->setTheme(theme);
    }
}

bool WebUiServer::start()
{
    if (running_.load(std::memory_order_acquire)) return true;

    listen_fd_ = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (listen_fd_ < 0) return false;

    // Set SO_REUSEADDR to avoid "address in use" errors on restart
    int opt = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = ::htons(config_.port);
    if (::inet_pton(AF_INET, config_.bind_address.c_str(), &addr.sin_addr) != 1) {
        ::close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }

    if (::bind(listen_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }

    if (::listen(listen_fd_, kListenBacklog) < 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }

    socklen_t address_size = sizeof(addr);
    if (::getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&addr), &address_size) == 0) {
        config_.port = ::ntohs(addr.sin_port);
    }
    workers_.reserve(8);
    running_.store(true, std::memory_order_release);
    thread_ = std::thread(&WebUiServer::run, this);
    return true;
}

void WebUiServer::stop()
{
    running_.store(false, std::memory_order_release);
    // Keep the descriptor stable until the accept thread has exited.
    if (listen_fd_ >= 0) ::shutdown(listen_fd_, SHUT_RDWR);
    if (thread_.joinable()) thread_.join();
    for (const auto& worker : workers_) {
        worker->cancellation.request_stop();
        const std::lock_guard lock{worker->socket_mutex};
        if (worker->fd >= 0) ::shutdown(worker->fd, SHUT_RDWR);
    }
    for (const auto& worker : workers_) {
        if (worker->thread.joinable()) worker->thread.join();
    }
    workers_.clear();
    if (listen_fd_ >= 0) ::close(listen_fd_);
    listen_fd_ = -1;
}

bool WebUiServer::isRunning() const noexcept
{
    return running_.load(std::memory_order_acquire);
}

const std::string& WebUiServer::bindAddress() const noexcept
{
    return config_.bind_address;
}

std::uint16_t WebUiServer::port() const noexcept
{
    return config_.port;
}

void WebUiServer::run()
{
    while (running_.load(std::memory_order_acquire)) {
        const int fd = ::accept4(listen_fd_, nullptr, nullptr, SOCK_CLOEXEC);
        if (fd < 0) {
            if (!running_.load(std::memory_order_acquire)) break;
            if (errno != EINTR) std::this_thread::sleep_for(std::chrono::milliseconds{10});
            continue;
        }
        std::erase_if(workers_, [](const auto& worker) {
            if (!worker->finished.load(std::memory_order_acquire)) return false;
            worker->thread.join();
            return true;
        });
        if (!running_.load(std::memory_order_acquire) || workers_.size() >= 8) {
            constexpr std::string_view busy = "HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            ::send(fd, busy.data(), busy.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
            ::close(fd);
            continue;
        }
        workers_.push_back(std::make_unique<Worker>());
        auto* worker = workers_.back().get();
        worker->fd = fd;
        try {
            worker->thread = std::thread([this, worker, fd] {
                const core::OperationCancellationScope cancellation{worker->cancellation.get_token()};
                try { handleClient(fd); }
                catch (...) { /* Connection cleanup must also cover provider exceptions. */ }
                {
                    const std::lock_guard lock{worker->socket_mutex};
                    ::close(worker->fd);
                    worker->fd = -1;
                }
                worker->finished.store(true, std::memory_order_release);
            });
        } catch (...) {
            ::close(fd);
            workers_.pop_back();
        }
    }
}

void WebUiServer::handleClient(int client_fd)
{
    const auto request = readHttpRequest(client_fd);
    if (request.empty()) {
        (void)sendSocketBytes(client_fd, json::httpBadRequest("Incomplete or invalid HTTP request"));
        return;
    }
    const auto line_end = request.find("\r\n");
    if (line_end == std::string::npos) return;
    const std::string_view request_line{request.data(), line_end};
    const auto method = parseMethod(request_line);
    const auto path = parsePath(request_line);
    const auto header_end = request.find("\r\n\r\n");
    const auto headers = std::string_view{request}.substr(0, header_end + 4);
    if (method == "GET" && path == "/api/runtime/events" && isWebSocketUpgrade(headers)) {
        // The stream owns its duplicate; the worker retains a descriptor that
        // stop() can shutdown without racing descriptor close/reuse.
        const int stream_fd = ::fcntl(client_fd, F_DUPFD_CLOEXEC, 0);
        if (stream_fd < 0) return;
        WebUiWsRuntimeStream stream{stream_fd, adapter_};
        if (stream.performUpgrade(headers)) stream.run();
        return;
    }
    const auto body = std::string_view{request}.substr(header_end + 4);
    (void)sendSocketBytes(client_fd, router_->handleRequest(method, path, body));
}

std::string WebUiServer::readHttpRequest(int fd)
{
    constexpr std::size_t maximum = 65536;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
    std::string buffer;
    buffer.reserve(kReadBufferSize);
    std::optional<std::size_t> expected;
    while (running_.load(std::memory_order_acquire) && std::chrono::steady_clock::now() < deadline) {
        pollfd pending{fd, POLLIN, 0};
        const int ready = ::poll(&pending, 1, 100);
        if (ready < 0 && errno == EINTR) continue;
        if (ready < 0) return {};
        if (ready == 0) continue;
        char chunk[kReadBufferSize];
        const auto count = ::recv(fd, chunk, sizeof(chunk), MSG_DONTWAIT);
        if (count < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) continue;
        if (count <= 0) return {};
        buffer.append(chunk, static_cast<std::size_t>(count));
        if (buffer.size() > maximum) return {};
        const auto header_end = buffer.find("\r\n\r\n");
        if (!expected && header_end != std::string::npos) {
            std::size_t length = 0;
            bool has_length = false;
            auto position = buffer.find("\r\n") + 2;
            while (position < header_end) {
                const auto end = buffer.find("\r\n", position);
                const auto line = std::string_view{buffer}.substr(position, end - position);
                const auto colon = line.find(':');
                if (colon == std::string_view::npos) return {};
                const auto name = line.substr(0, colon);
                if (sameHeader(name, "Transfer-Encoding")) return {}; // no chunked request bodies
                if (sameHeader(name, "Content-Length")) {
                    if (has_length) return {};
                    const auto value = trimHeader(line.substr(colon + 1));
                    const auto [ptr, error] = std::from_chars(value.data(), value.data() + value.size(), length);
                    if (error != std::errc{} || ptr != value.data() + value.size()) return {};
                    has_length = true;
                }
                position = end + 2;
            }
            if (length > maximum - header_end - 4) return {};
            expected = header_end + 4 + length;
        }
        if (expected && buffer.size() >= *expected) {
            buffer.resize(*expected);
            return buffer;
        }
    }
    return {};
}

} // namespace lofibox::webui
