// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/host/runtime_provider_factories.h"

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

namespace {
namespace fs = std::filesystem;
using namespace std::chrono_literals;
volatile std::sig_atomic_t terminating = 0;

void onTerm(int) { terminating = 1; }

// Run this same executable through ffmpeg/pw-cat symlinks. The children use
// real pipes, signals, pause/resume and delayed termination, with no speakers.
int childMode(const std::string& name)
{
    std::signal(SIGTERM, onTerm);
    std::signal(SIGPIPE, SIG_IGN);
    const fs::path root{std::getenv("LOFIBOX_RESTART_HELPER_DIR")};
    std::ofstream(root / ("pid-" + std::to_string(getpid()))) << name;
    const bool decoder = name == "ffmpeg";
    const int fd = decoder ? STDOUT_FILENO : STDIN_FILENO;
    const int flags = fcntl(fd, F_GETFL, 0);
    (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    std::array<char, 4096> pcm{};
    auto finish = std::chrono::steady_clock::time_point::max();
    while (true) {
        if (terminating && finish == std::chrono::steady_clock::time_point::max())
            finish = std::chrono::steady_clock::now() + 180ms;
        if (std::chrono::steady_clock::now() >= finish) return 0;
        if (decoder) (void)write(fd, pcm.data(), pcm.size());
        else (void)read(fd, pcm.data(), pcm.size());
        std::this_thread::sleep_for(1ms);
    }
}

void expect(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}

std::vector<pid_t> childPids(const fs::path& root)
{
    std::vector<pid_t> result;
    for (const auto& entry : fs::directory_iterator(root)) {
        const auto name = entry.path().filename().string();
        if (name.starts_with("pid-")) result.push_back(static_cast<pid_t>(std::stoi(name.substr(4))));
    }
    return result;
}

std::size_t descriptors()
{
    return static_cast<std::size_t>(std::distance(fs::directory_iterator{"/proc/self/fd"}, fs::directory_iterator{}));
}

struct ChildCleanup {
    fs::path root;
    ~ChildCleanup()
    {
        for (const auto pid : childPids(root)) if (kill(pid, 0) == 0) {
            kill(-pid, SIGKILL);kill(pid, SIGKILL);
        }
        std::error_code error;fs::remove_all(root,error);
    }
};
} // namespace

int main(int argc, char** argv)
{
    const std::string name = fs::path{argv[0]}.filename().string();
    if (name == "ffmpeg" || name == "pw-cat" || name == "ffplay") return childMode(name);
    (void)argc;
    try {
        std::array<char, 64> pattern{};
        const std::string prefix = "/tmp/lofibox-audio-restart-XXXXXX";
        std::copy(prefix.begin(),prefix.end(),pattern.begin());
        const char* created = mkdtemp(pattern.data());
        expect(created != nullptr, "failed to create unique test directory");
        const fs::path root{created};ChildCleanup cleanup{root};
        const auto self = fs::canonical("/proc/self/exe");
        for (const auto* helper : {"ffmpeg", "ffplay", "pw-cat"}) fs::create_symlink(self,root/helper);
        std::ofstream(root / "pipewire-0");
        std::ofstream(root / "input.wav");
        setenv("PATH",root.c_str(),1);
        setenv("FFMPEG_PATH",(root/"ffmpeg").c_str(),1);
        setenv("FFPLAY_PATH",(root/"ffplay").c_str(),1);
        setenv("PIPEWIRE_RUNTIME_DIR",root.c_str(),1);
        setenv("LOFIBOX_RESTART_HELPER_DIR",root.c_str(),1);

        const auto before_fds = descriptors();
        auto backend = lofibox::platform::host::createHostAudioPlaybackBackend();
        expect(backend->available(), "fake PCM backend unavailable");
        expect(backend->playFile(root/"input.wav",0), "initial play failed");
        const auto ready = std::chrono::steady_clock::now()+3s;
        while (backend->state()!=lofibox::app::AudioPlaybackState::Playing) {
            expect(std::chrono::steady_clock::now()<ready,"PCM output failed to start");
            std::this_thread::sleep_for(5ms);
        }
        backend->pause();std::this_thread::sleep_for(20ms);
        const auto replacing_paused = std::chrono::steady_clock::now();
        expect(backend->playFile(root/"input.wav",0), "replacement of paused track failed");
        expect(std::chrono::steady_clock::now()-replacing_paused<300ms,
               "replacing paused playback waited for child termination");

        std::atomic<int> descriptor_errors{0};
        std::jthread churn{[&](std::stop_token token) {
            while(!token.stop_requested()) {
                int fds[2];
                if(pipe2(fds,O_CLOEXEC|O_NONBLOCK)!=0) {++descriptor_errors;continue;}
                const char marker='x';char received=0;
                if(write(fds[1],&marker,1)!=1 || read(fds[0],&received,1)!=1 || received!=marker)
                    ++descriptor_errors;
                close(fds[0]);close(fds[1]);
                std::this_thread::sleep_for(100us);
            }
        }};

        std::chrono::steady_clock::duration slowest{};
        for (int index=0;index<24;++index) {
            const auto start=std::chrono::steady_clock::now();
            expect(backend->playFile(root/"input.wav",0),"rapid restart failed");
            slowest=std::max(slowest,std::chrono::steady_clock::now()-start);
            // Two retiring pipelines plus the current one: at most six child
            // processes, including zombies awaiting their owner's next poll.
            int present=0;
            for (const auto pid:childPids(root)) if (kill(pid,0)==0) ++present;
            expect(present<=6,"rapid restarts exceeded the bounded child budget");
            std::this_thread::sleep_for(10ms);
        }
        expect(slowest<600ms,"bounded retirement caused excessive restart delay");
        // The stop command must not join delayed child cleanup. Descriptors
        // and children must still be reclaimed while the backend stays alive.
        const auto settled = std::chrono::steady_clock::now()+3s;
        while (true) {
            int present=0;
            for (const auto pid:childPids(root)) if (kill(pid,0)==0) ++present;
            if (present==2) break;
            expect(std::chrono::steady_clock::now()<settled,"retired children did not settle");
            std::this_thread::sleep_for(5ms);
        }
        backend->pause();
        const auto stopping = std::chrono::steady_clock::now();
        backend->stop();
        const auto stop_delay = std::chrono::steady_clock::now()-stopping;
        expect(stop_delay<100ms,"stop command joined delayed child cleanup");
        expect(backend->state()==lofibox::app::AudioPlaybackState::Idle,"stopped backend did not report idle immediately");
        const auto reclaimed = std::chrono::steady_clock::now()+3s;
        while (true) {
            bool present=false;
            for (const auto pid:childPids(root)) if (kill(pid,0)==0) present=true;
            if (!present) break;
            expect(std::chrono::steady_clock::now()<reclaimed,"stop retained children while backend remained alive");
            std::this_thread::sleep_for(5ms);
        }
        churn.request_stop();churn.join();
        expect(descriptor_errors==0,"audio cleanup closed a reused unrelated descriptor");
        expect(descriptors()<=before_fds+1,"asynchronous stop retained file descriptors");
        for (int index=0;index<12;++index) {
            expect(backend->playFile(root/"input.wav",0),"play after asynchronous stop failed");
            const auto start=std::chrono::steady_clock::now();
            backend->stop();
            expect(std::chrono::steady_clock::now()-start<100ms,"burst stop waited for retirement capacity");
            int present=0;
            for (const auto pid:childPids(root)) if (kill(pid,0)==0) ++present;
            expect(present<=6,"play/stop bursts exceeded the six-child budget");
            std::this_thread::sleep_for(10ms);
        }
        // Destruction is the synchronous ownership boundary, including when a
        // user exits immediately after stopping while children are retiring.
        backend.reset();
        for (const auto pid:childPids(root)) expect(kill(pid,0)!=0,"playback child survived stop/join");
        expect(descriptors()<=before_fds+1,"restart/stop leaked file descriptors");
        std::cout << "Paused replacement returns before cleanup; 24 restarts retain <=6 children and reap all. "
                  << "Slowest restart: " << std::chrono::duration_cast<std::chrono::milliseconds>(slowest).count() << " ms. "
                  << "Paused stop: " << std::chrono::duration_cast<std::chrono::microseconds>(stop_delay).count() << " us.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';return 1;
    }
}
