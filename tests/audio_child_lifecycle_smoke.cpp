// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <thread>

#if defined(__linux__)
#include <csignal>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "platform/host/runtime_host_internal.h"
#endif

namespace fs = std::filesystem;

namespace {

#if defined(__linux__)
bool processExists(pid_t pid)
{
    if (pid <= 0) {
        return false;
    }
    if (kill(pid, 0) == 0) {
        return true;
    }
    return errno != ESRCH;
}

bool waitUntilGone(pid_t pid, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (!processExists(pid)) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{25});
    }
    return !processExists(pid);
}
#endif

} // namespace

int main()
{
#if !defined(__linux__)
    std::cout << "audio child lifecycle smoke is Linux-only.\n";
    return 0;
#else
    int pipe_fds[2]{-1, -1};
    if (pipe(pipe_fds) != 0) {
        std::cerr << "Failed to create pid pipe: " << std::strerror(errno) << "\n";
        return 1;
    }

    const pid_t parent = fork();
    if (parent < 0) {
        std::cerr << "Failed to fork lifecycle parent: " << std::strerror(errno) << "\n";
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        return 1;
    }

    if (parent == 0) {
        close(pipe_fds[0]);
        lofibox::platform::host::runtime_detail::RunningProcess child{};
        const bool spawned = lofibox::platform::host::runtime_detail::spawnAudioProcess(
            child,
            fs::path{"/bin/sleep"},
            {"60"});
        const pid_t spawned_pid = spawned ? child.pid : -1;
        const ssize_t written = write(pipe_fds[1], &spawned_pid, sizeof(spawned_pid));
        close(pipe_fds[1]);
        if (written != sizeof(spawned_pid)) {
            _exit(3);
        }
        if (!spawned) {
            _exit(2);
        }
        for (;;) {
            pause();
        }
    }

    close(pipe_fds[1]);
    pid_t spawned_pid{-1};
    const ssize_t read_bytes = read(pipe_fds[0], &spawned_pid, sizeof(spawned_pid));
    close(pipe_fds[0]);
    if (read_bytes != sizeof(spawned_pid) || spawned_pid <= 0) {
        kill(parent, SIGKILL);
        waitpid(parent, nullptr, 0);
        std::cerr << "Failed to receive spawned child pid.\n";
        return 1;
    }

    if (!processExists(spawned_pid)) {
        kill(parent, SIGKILL);
        waitpid(parent, nullptr, 0);
        std::cerr << "Expected spawned child process to be alive before parent termination.\n";
        return 1;
    }

    kill(parent, SIGTERM);
    waitpid(parent, nullptr, 0);
    if (!waitUntilGone(spawned_pid, std::chrono::seconds{3})) {
        kill(-spawned_pid, SIGKILL);
        kill(spawned_pid, SIGKILL);
        std::cerr << "Expected spawned playback child to exit when its LoFiBox parent is killed.\n";
        return 1;
    }

    return 0;
#endif
}
