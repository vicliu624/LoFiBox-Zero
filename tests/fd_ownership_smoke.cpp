// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/host/runtime_host_internal.h"
#include "platform/device/linux_framebuffer.h"
#include "plugins/plugin_runtime.h"

#include <array>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
namespace fs = std::filesystem;
using namespace std::chrono_literals;
void expect(bool value,const char* message) { if(!value) throw std::runtime_error(message); }
std::size_t descriptors() { return static_cast<std::size_t>(std::distance(fs::directory_iterator{"/proc/self/fd"},fs::directory_iterator{})); }

int helper(int argc,char** argv)
{
    struct stat canary{};
    if (stat(argv[2],&canary)!=0) return 2;
    std::string result="CLEAN\n";
    for(int fd=3;fd<1024;++fd) {
        struct stat candidate{};
        if(fstat(fd,&candidate)==0 && candidate.st_dev==canary.st_dev && candidate.st_ino==canary.st_ino)
            result="INHERITED\n";
    }
    if(argc>3) std::ofstream(argv[3]) << result;
    else std::cout << result << std::flush;
    if(std::string{argv[1]}=="--plugin") {
        std::cout << "{\"jsonrpc\":\"2.0\",\"method\":\"ready\"}\n" << std::flush;
        std::string request;while(std::getline(std::cin,request)) {}
    }
    return result=="CLEAN\n"?0:3;
}

std::string contents(const fs::path& path) { std::ifstream input(path);return {std::istreambuf_iterator<char>{input},{}}; }
}

int main(int argc,char** argv)
{
    if(argc>=3 && std::string{argv[1]}.starts_with("--")) return helper(argc,argv);
    try {
        std::array<char,64> pattern{};const std::string prefix="/tmp/lofibox-fd-ownership-XXXXXX";
        std::copy(prefix.begin(),prefix.end(),pattern.begin());
        const auto* created=mkdtemp(pattern.data());expect(created,"mkdtemp failed");
        const fs::path root{created};const auto canary_path=root/"canary";
        const int canary=open(canary_path.c_str(),O_CREAT|O_RDWR,0600);
        expect(canary>=3,"canary descriptor unavailable");
        const auto self=fs::canonical("/proc/self/exe");
        namespace detail=lofibox::platform::host::runtime_detail;
        const auto captured=detail::captureProcessOutput(self,{"--report",canary_path.string()});
        expect(captured && *captured=="CLEAN\n","capture helper inherited unrelated parent FD");
        expect(detail::runProcess(self,{"--report",canary_path.string(),(root/"run").string()}),"runProcess inherited unrelated parent FD");
        expect(contents(root/"run")=="CLEAN\n","runProcess report invalid");
        detail::RunningPipeProcess reader{};
        expect(detail::spawnPipeProcess(reader,self,{"--report",canary_path.string()}),"pipe spawn failed");
        expect((fcntl(reader.read_fd,F_GETFD)&FD_CLOEXEC)!=0,"decoder pipe lacks CLOEXEC");
        std::string output;
        for(int attempt=0;attempt<200;++attempt) {
            char bytes[64];const auto count=detail::readPipeProcess(reader,bytes,sizeof(bytes));
            if(count>0) output.append(bytes,count);
            if(count==0) break;
            std::this_thread::sleep_for(2ms);
        }
        detail::stopPipeProcess(reader);expect(output=="CLEAN\n","decoder inherited unrelated parent FD");
        detail::RunningInputProcess writer{};
        expect(detail::spawnInputProcess(writer,self,{"--report",canary_path.string(),(root/"input").string()}),"input spawn failed");
        expect((fcntl(writer.write_fd,F_GETFD)&FD_CLOEXEC)!=0,"sink pipe lacks CLOEXEC");
        const auto deadline=std::chrono::steady_clock::now()+2s;
        while(detail::inputProcessRunning(writer) && std::chrono::steady_clock::now()<deadline) std::this_thread::sleep_for(2ms);
        detail::stopInputProcess(writer);expect(contents(root/"input")=="CLEAN\n","sink inherited unrelated parent FD");
        detail::RunningProcess audio{};
        expect(detail::spawnAudioProcess(audio,self,{"--report",canary_path.string(),(root/"audio").string()}),"audio spawn failed");
        for(int attempt=0;attempt<200 && detail::audioProcessRunning(audio);++attempt) std::this_thread::sleep_for(2ms);
        detail::stopAudioProcess(audio);expect(contents(root/"audio")=="CLEAN\n","audio child inherited unrelated parent FD");
        lofibox::plugins::PluginRuntimeConfig config{};
        config.plugin_dir=root;config.stderr_log_dir=root/"logs";config.command=self.string();
        config.args={"--plugin",canary_path.string(),(root/"plugin").string()};config.ready_timeout=2s;
        {lofibox::plugins::PluginRuntime plugin{config};expect(plugin.start(),"plugin start failed");plugin.stop();}
        expect(contents(root/"plugin")=="CLEAN\n","plugin inherited unrelated parent FD");

        const auto baseline=descriptors();
        for(int attempt=0;attempt<100;++attempt) {
            bool rejected=false;
            try {lofibox::platform::device::LinuxFramebuffer bad{"/dev/null"};}
            catch(const std::exception&) {rejected=true;}
            expect(rejected,"non-framebuffer device accepted");
            expect(!detail::captureProcessOutput(root/"missing",{}),"missing helper accepted");
            detail::RunningPipeProcess missing{};
            expect(!detail::spawnPipeProcess(missing,root/"missing",{}),"missing decoder accepted");
            detail::RunningInputProcess missing_sink{};
            expect(!detail::spawnInputProcess(missing_sink,root/"missing",{}),"missing sink accepted");
        }
        expect(descriptors()==baseline,"failure paths leaked descriptors");
        expect(fcntl(canary,F_GETFD)>=0,"unrelated parent descriptor was closed");
        close(canary);fs::remove_all(root);
        std::cout << "Five spawn paths exclude parent FDs; pipes are CLOEXEC; 100 framebuffer/spawn failures retain no FDs.\n";
        return 0;
    } catch(const std::exception& error) {std::cerr << error.what() << '\n';return 1;}
}
