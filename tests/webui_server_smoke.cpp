// SPDX-License-Identifier: GPL-3.0-or-later
#include "webui/webui_server.h"
#include "webui/webui_runtime_adapter.h"
#include "runtime/runtime_command_client.h"
#include "app/library_enrich_provider.h"
#include "core/operation_cancellation.h"
#include "platform/host/runtime_host_internal.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
using namespace std::chrono_literals;
void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

class Client : public lofibox::runtime::RuntimeCommandClient {
public:
    std::atomic<int> generation{1};
    std::atomic<int> commands{0};
    lofibox::runtime::RuntimeCommandResult dispatch(const lofibox::runtime::RuntimeCommand&) override
    {
        ++commands;
        lofibox::runtime::RuntimeCommandResult result{};
        result.accepted = result.applied = true;
        result.code = "ok";
        return result;
    }
    lofibox::runtime::RuntimeSnapshot query(const lofibox::runtime::RuntimeQuery&) const override
    {
        const int version = generation.load();
        lofibox::runtime::RuntimeSnapshot result{};
        result.version = version;
        result.playback.title = "generation" + std::to_string(version);
        return result;
    }
};

class SlowEnrichment : public lofibox::app::LibraryEnrichProvider {
public:
    std::filesystem::path ready = std::filesystem::temp_directory_path()
        / ("lofibox-slow-web-request-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    mutable std::atomic<bool> cancelled{false};
    ~SlowEnrichment() { std::error_code error; std::filesystem::remove(ready, error); }
    std::optional<lofibox::app::ArtistEnrichInfo> getArtistInfo(std::string_view) const override
    {
        (void)lofibox::platform::host::runtime_detail::captureProcessOutput(
            "/bin/sh", {"-c", "printf ready > \"$1\"; sleep 8", "lofibox-test", ready.string()});
        cancelled = lofibox::core::operationCancelled();
        return std::nullopt;
    }
    std::optional<lofibox::app::AlbumEnrichInfo> getAlbumInfo(std::string_view, std::string_view) const override { return std::nullopt; }
};

struct Socket {
    int fd{-1};
    explicit Socket(std::uint16_t port)
    {
        fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
        expect(fd >= 0, "socket failed");
        timeval timeout{2, 0};
        ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(port);
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
            ::close(std::exchange(fd, -1));
            throw std::runtime_error("connect failed");
        }
    }
    Socket(Socket&& other) noexcept : fd(std::exchange(other.fd, -1)) {}
    Socket(const Socket&) = delete;
    ~Socket() { if (fd >= 0) ::close(fd); }
    void send(std::string_view bytes)
    {
        while (!bytes.empty()) {
            const auto sent = ::send(fd, bytes.data(), bytes.size(), MSG_NOSIGNAL);
            expect(sent > 0, "send failed");
            bytes.remove_prefix(sent);
        }
    }
    std::string read(std::size_t count)
    {
        std::string output(count, '\0');
        std::size_t position = 0;
        while (position < count) {
            const auto bytes = ::recv(fd, output.data() + position, count - position, 0);
            expect(bytes > 0, "read failed or timed out");
            position += bytes;
        }
        return output;
    }
    std::string response()
    {
        std::string output;
        char bytes[4096];
        while (true) {
            const auto count = ::recv(fd, bytes, sizeof(bytes), 0);
            if (count == 0) return output;
            expect(count > 0, "response timed out");
            output.append(bytes, count);
        }
    }
    void upgrade()
    {
        send("GET /api/runtime/events HTTP/1.1\r\nHost: localhost\r\nConnection: Upgrade\r\n"
             "Upgrade: WebSocket\r\nSec-WebSocket-Version: 13\r\n"
             "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n");
        std::string headers;
        while (!headers.ends_with("\r\n\r\n")) headers += read(1);
        expect(headers.find("101 Switching Protocols") != std::string::npos, "upgrade failed");
        expect(headers.find("s3pPLMBiTxaQ9kYGzzhZRbK+xOo=") != std::string::npos, "incorrect WebSocket accept key");
    }
    std::pair<unsigned, std::string> frame()
    {
        const auto head = read(2);
        const unsigned opcode = static_cast<unsigned char>(head[0]) & 15U;
        std::uint64_t length = static_cast<unsigned char>(head[1]) & 127U;
        if (length >= 126) {
            const auto extended = read(length == 126 ? 2 : 8);
            length = 0;
            for (unsigned char byte : extended) length = (length << 8) | byte;
        }
        expect(length < 1000000, "unbounded WebSocket frame");
        return {opcode, read(length)};
    }
    void until(std::string_view content)
    {
        for (int attempt = 0; attempt < 40; ++attempt) {
            if (frame().second.find(content) != std::string::npos) return;
        }
        throw std::runtime_error("WebSocket missed an update");
    }
};

std::size_t threadCount()
{
    return std::distance(std::filesystem::directory_iterator{"/proc/self/task"}, std::filesystem::directory_iterator{});
}
} // namespace

int main()
{
    try {
        const auto baseline_threads = threadCount();
        Client client;
        lofibox::webui::WebUiRuntimeAdapter adapter{client};
        lofibox::webui::WebUiConfig config{};
        config.bind_address = "127.0.0.1";
        config.port = 0;
        lofibox::webui::WebUiServer server{config, adapter};
        expect(server.start(), "server did not start");
        const auto port = server.port();
        {
            Socket connection{port};
            const std::string body = "{\"kind\":\"webui.Toggle\"}";
            connection.send("POST /api/runtime/commands HTTP/1.1\r\nHost: localhost\r\ncOnTeNt-LeNgTh: "
                + std::to_string(body.size()) + "\r\n\r\n");
            std::this_thread::sleep_for(30ms);
            connection.send(body.substr(0, 7));
            std::this_thread::sleep_for(30ms);
            connection.send(body.substr(7));
            expect(connection.response().find("200 OK") != std::string::npos, "split command body was rejected");
            expect(client.commands == 1, "split command was not dispatched exactly once");
        }
        for (const auto length : {"-1", "9999999999999999999999", "1junk"}) {
            Socket invalid{port};
            invalid.send(std::string{"POST /api/runtime/commands HTTP/1.1\r\nContent-Length: "} + length + "\r\n\r\n");
            expect(invalid.response().find("400 Bad Request") != std::string::npos, "invalid request length accepted");
        }
        {
            Socket slow{port};
            slow.send("GET / HTTP/1.1\r\n");
            Socket healthy{port};
            const auto start = std::chrono::steady_clock::now();
            healthy.send("GET /api/runtime/snapshot HTTP/1.1\r\nHost: localhost\r\n\r\n");
            expect(healthy.response().find("200 OK") != std::string::npos, "slow client blocked a healthy client");
            expect(std::chrono::steady_clock::now() - start < 1s, "healthy client was delayed by a slow request");
        }
        {
            Socket first{port}, second{port};
            first.upgrade(); second.upgrade();
            first.until("generation1"); second.until("generation1");
            client.generation = 2;
            first.until("generation2"); second.until("generation2");
            // Masked ping: payload 'hi', mask bytes 1,2,3,4.
            const char ping[] = {char(0x89), char(0x82), 1, 2, 3, 4, char('h' ^ 1), char('i' ^ 2)};
            first.send(std::string_view{ping, sizeof(ping)});
            bool pong = false;
            for (int attempt = 0; attempt < 40 && !pong; ++attempt) {
                const auto [opcode, payload] = first.frame();
                pong = opcode == 0xa && payload == "hi";
            }
            expect(pong, "WebSocket ping was not acknowledged");
        }
        std::this_thread::sleep_for(200ms);
        // Many incomplete requests must not create an unbounded thread count.
        std::vector<Socket> held;
        for (int i = 0; i < 16; ++i) {
            held.emplace_back(port);
            held.back().send("GET / HTTP/1.1\r\n");
        }
        std::this_thread::sleep_for(100ms);
        expect(threadCount() <= baseline_threads + 9, "connection workers are unbounded");
        const auto stopping = std::chrono::steady_clock::now();
        server.stop();
        expect(std::chrono::steady_clock::now() - stopping < 1s, "incomplete clients blocked shutdown");
        expect(threadCount() == baseline_threads, "server left worker threads alive");
        expect(server.start(), "server could not restart");
        server.stop();
        SlowEnrichment enrichment;
        server.setLibraryEnrichProvider(&enrichment);
        expect(server.start(), "server could not start for enrichment cancellation");
        Socket enriching{server.port()};
        enriching.send("GET /api/library/artist/Slow HTTP/1.1\r\nHost: localhost\r\n\r\n");
        const auto ready_deadline = std::chrono::steady_clock::now() + 2s;
        while (!std::filesystem::exists(enrichment.ready) && std::chrono::steady_clock::now() < ready_deadline) std::this_thread::sleep_for(5ms);
        expect(std::filesystem::exists(enrichment.ready), "slow helper did not start");
        const auto cancelling = std::chrono::steady_clock::now();
        server.stop();
        expect(std::chrono::steady_clock::now() - cancelling < 1s, "slow enrichment helper blocked shutdown");
        expect(enrichment.cancelled, "HTTP provider did not receive cancellation");
        std::cout << "Real HTTP fragmentation/concurrency/shutdown and WebSocket fanout/ping checks passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
