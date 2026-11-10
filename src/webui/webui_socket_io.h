// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cerrno>
#include <chrono>
#include <string_view>
#include <poll.h>
#include <sys/socket.h>

namespace lofibox::webui {

// A disconnected or slow browser must not terminate the player with SIGPIPE
// or keep an output worker blocked indefinitely. Handles short writes as well.
inline bool sendSocketBytes(int fd, std::string_view bytes)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};
    while (!bytes.empty() && std::chrono::steady_clock::now() < deadline) {
        const auto sent = ::send(fd, bytes.data(), bytes.size(), MSG_NOSIGNAL | MSG_DONTWAIT);
        if (sent > 0) {
            bytes.remove_prefix(static_cast<std::size_t>(sent));
        } else if (sent < 0 && errno == EINTR) {
            continue;
        } else if (sent < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            pollfd pending{fd, POLLOUT, 0};
            if (::poll(&pending, 1, 100) < 0 && errno != EINTR) return false;
            if (pending.revents & (POLLERR | POLLHUP | POLLNVAL)) return false;
        } else {
            return false;
        }
    }
    return bytes.empty();
}

} // namespace lofibox::webui
