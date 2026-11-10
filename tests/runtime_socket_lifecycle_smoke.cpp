// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/app_controller_set.h"
#include "runtime/runtime_host.h"
#include "runtime/unix_socket_runtime_transport.h"

#include <array>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace {
namespace fs=std::filesystem;
using namespace std::chrono_literals;
void expect(bool value,const char* message) {if(!value) throw std::runtime_error(message);}
std::size_t descriptors() {return static_cast<std::size_t>(std::distance(fs::directory_iterator{"/proc/self/fd"},fs::directory_iterator{}));}
long vmPages() {std::ifstream input("/proc/self/statm");long pages=0;input>>pages;return pages;}
int connectRaw(const fs::path& path)
{
    const int fd=socket(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0);expect(fd>=0,"client socket failed");
    sockaddr_un addr{};addr.sun_family=AF_UNIX;const auto name=path.string();
    expect(name.size()<sizeof(addr.sun_path),"test socket path too long");
    std::copy(name.begin(),name.end(),addr.sun_path);
    if(connect(fd,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))!=0) {close(fd);throw std::runtime_error("connect failed");}
    return fd;
}
}

int main()
{
    try {
        std::array<char,64> pattern{};const std::string prefix="/tmp/lofibox-runtime-socket-XXXXXX";
        std::copy(prefix.begin(),prefix.end(),pattern.begin());
        const char* created=mkdtemp(pattern.data());expect(created,"mkdtemp failed");const fs::path root{created};
        auto services=lofibox::app::withNullRuntimeServices();
        lofibox::app::AppControllerSet controllers;controllers.bindServices(services);
        lofibox::runtime::RuntimeHost host{lofibox::application::AppServiceRegistry{controllers,services}};
        const auto before=descriptors();
        expect(host.startExternalTransport(root/"runtime.sock"),"runtime server start failed");
        lofibox::runtime::UnixSocketRuntimeCommandClient client{root/"runtime.sock"};
        // Warm libc's thread stack/malloc arena caches before measuring retained
        // per-connection resources. Initial arenas are not connection leaks.
        for(int i=0;i<20;++i) (void)client.query({});
        std::this_thread::sleep_for(30ms);
        const auto before_vm=vmPages();
        for(int i=0;i<200;++i) {
            const auto snapshot=client.query({});
            expect(snapshot.version==0,"runtime query failed");
        }
        const auto vm_growth=(vmPages()-before_vm)*sysconf(_SC_PAGESIZE);
        std::cout << "Virtual memory growth after warmup: " << vm_growth << " bytes.\n";
        expect(vm_growth<64*1024*1024,"completed runtime threads retain unbounded stacks");
        std::this_thread::sleep_for(30ms);
        expect(descriptors()<=before+2,"completed runtime clients leaked sockets");
        lofibox::runtime::UnixSocketRuntimeEventStream events{root/"runtime.sock"};
        expect(events.connect(),"event subscription failed");
        expect(events.next(1s).has_value(),"event subscription produced no snapshot");
        events.close();

        // Hold incomplete frames open. stop must wake recv before joining.
        std::vector<int> silent;
        for(int i=0;i<4;++i) silent.push_back(connectRaw(root/"runtime.sock"));
        std::this_thread::sleep_for(30ms);
        const auto start=std::chrono::steady_clock::now();
        auto stopped=std::async(std::launch::async,[&] {host.stopExternalTransport();});
        const bool prompt=stopped.wait_for(600ms)==std::future_status::ready;
        for(const auto fd:silent) close(fd);
        stopped.get();
        expect(prompt,"server stop waited for incomplete client frames");
        expect(std::chrono::steady_clock::now()-start<1s,"server shutdown too slow");
        expect(descriptors()==before,"server shutdown leaked descriptors");
        expect(host.startExternalTransport(root/"runtime.sock"),"server restart failed");
        (void)client.query({});host.stopExternalTransport();
        expect(descriptors()==before,"server restart leaked descriptors");
        fs::remove_all(root);
        std::cout << "200 socket queries reclaim threads/FDs; event disconnect, silent-client shutdown and restart release all FDs.\n";
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
