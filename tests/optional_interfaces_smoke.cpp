// SPDX-License-Identifier: GPL-3.0-or-later
#include "targets/webui_starter.h"
#include "app/app_runtime_context.h"
#include "runtime/unix_socket_runtime_transport.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <thread>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
namespace {
using namespace std::chrono_literals;
namespace fs = std::filesystem;
std::size_t fds() { return std::distance(fs::directory_iterator{"/proc/self/fd"}, fs::directory_iterator{}); }
template<class P> void waitFor(P predicate) {
    const auto deadline = std::chrono::steady_clock::now() + 3s;
    while (!predicate()) { assert(std::chrono::steady_clock::now() < deadline); std::this_thread::sleep_for(5ms); }
}
bool listening(unsigned short port) {
    const int fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    sockaddr_in address{}; address.sin_family = AF_INET; address.sin_addr.s_addr = htonl(INADDR_LOOPBACK); address.sin_port = htons(port);
    const bool ok = connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
    close(fd); return ok;
}
}
int main() {
    using namespace lofibox;
    const auto root = fs::temp_directory_path() / ("lofibox-options-" + std::to_string(getpid()));
    fs::create_directories(root); setenv("XDG_RUNTIME_DIR", root.c_str(), 1);
    auto services = app::withNullRuntimeServices(); services.ui.framebuffer_backlight = true;
    services.cache.cache_manager = std::make_shared<cache::CacheManager>(root / "cache");
    const auto preferences = services.ui.preferences;
    {
        application::AppServiceHost host{services}; runtime::RuntimeHost runtime{host.registry()};
        app::AppRuntimeContext gui{{}, {}, {}, host, runtime.client()}; gui.openSettingsPage();
        for (const auto* name : {"LONG BATTERY", "SPECTRUM", "LEDS", "LED ANIMATION", "CLI / TUI"}) {
            const auto rows = gui.pageModel().rows;
            const auto found = std::find_if(rows.begin(), rows.end(), [name](const auto& row) { return row.first == name; });
            assert(found != rows.end()); gui.navigationState().list_selection.selected = std::distance(rows.begin(), found);
            const auto before = preferences->configured();
            gui.handleInput(app::InputEvent{app::InputKey::Character, "6", '6'});
            assert(preferences->configured() == (before ^ static_cast<unsigned>(*app::preferenceForRow(name))));
            gui.handleInput(app::InputEvent{app::InputKey::Enter, "ENTER"}); assert(preferences->configured() == before);
        }
    }
    const int reservation = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    sockaddr_in address{}; address.sin_family = AF_INET; address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    assert(bind(reservation, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0);
    socklen_t size = sizeof(address); assert(getsockname(reservation, reinterpret_cast<sockaddr*>(&address), &size) == 0);
    const auto port = ntohs(address.sin_port); close(reservation);
    const auto before = fds();
    {
        auto starter = targets::makeWebUiStarter({true, "127.0.0.1", port}, services);
        app::LoFiBoxApp gui{{}, {}, services, {}, true, std::move(starter)};
        const auto socket_path = runtime::defaultRuntimeSocketPath();
        waitFor([&] { return fs::exists(socket_path) && listening(port); });
        for (int cycle = 0; cycle < 20; ++cycle) {
            preferences->toggle(app::RuntimeFeature::BatterySaver);
            waitFor([&] { return !fs::exists(socket_path) && !listening(port); });
            gui.update(); core::Canvas canvas{320, 170}; gui.render(canvas);
            preferences->toggle(app::RuntimeFeature::BatterySaver);
            waitFor([&] { return fs::exists(socket_path) && listening(port); });
        }
        preferences->toggle(app::RuntimeFeature::WebUi); waitFor([&] { return !listening(port); }); assert(fs::exists(socket_path));
        preferences->toggle(app::RuntimeFeature::ExternalControl); waitFor([&] { return !fs::exists(socket_path); }); gui.update();
    }
    assert(fds() == before); fs::remove_all(root);
}
