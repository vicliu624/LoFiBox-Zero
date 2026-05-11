// SPDX-License-Identifier: GPL-3.0-or-later

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "app/app_runtime_context.h"
#include "app/input_event.h"
#include "app/runtime_services.h"
#include "application/app_service_host.h"
#include "midi/midi_port.h"
#include "runtime/runtime_host.h"
#include "ui/ui_theme.h"

namespace fs = std::filesystem;

namespace {

class FakeAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "FAKE"; }

    bool playFile(const std::filesystem::path& path, double) override
    {
        last_file = path;
        playing = true;
        ++play_file_calls;
        return true;
    }

    void stop() override
    {
        playing = false;
        ++stop_calls;
    }

    [[nodiscard]] bool isPlaying() override { return playing; }
    [[nodiscard]] bool isFinished() override { return false; }

    int play_file_calls{0};
    int stop_calls{0};
    fs::path last_file{};

private:
    bool playing{false};
};

class FakeMidiPort final : public lofibox::midi::MidiPort {
public:
    [[nodiscard]] bool open(std::string* error = nullptr) override
    {
        (void)error;
        opened = true;
        ++open_calls;
        status_message = "fake MIDI open";
        return true;
    }

    void close() noexcept override { opened = false; }
    [[nodiscard]] bool available() const noexcept override { return opened; }

    [[nodiscard]] lofibox::midi::MidiPortStatus status() const override
    {
        return lofibox::midi::MidiPortStatus{opened, "fake-in", "fake-out", status_message};
    }

    [[nodiscard]] std::vector<lofibox::midi::MidiMessage> poll(std::size_t max_messages = 64) override
    {
        std::vector<lofibox::midi::MidiMessage> out{};
        while (!incoming.empty() && out.size() < max_messages) {
            out.push_back(incoming.front());
            incoming.erase(incoming.begin());
        }
        return out;
    }

    [[nodiscard]] bool send(const lofibox::midi::MidiMessage& message) override
    {
        if (!opened) {
            return false;
        }
        sent.push_back(message);
        return true;
    }

    bool opened{false};
    int open_calls{0};
    std::string status_message{};
    std::vector<lofibox::midi::MidiMessage> incoming{};
    std::vector<lofibox::midi::MidiMessage> sent{};
};

void setEnvPath(const char* name, const fs::path& path)
{
#if defined(_WIN32)
    _putenv_s(name, path.string().c_str());
#else
    setenv(name, path.string().c_str(), 1);
#endif
}

void press(lofibox::app::AppRuntimeContext& app, lofibox::app::InputKey key, const char* label = "")
{
    app.handleInput(lofibox::app::InputEvent{key, label, '\0'});
}

bool sentType(const FakeMidiPort& port, lofibox::midi::MidiMessageType type)
{
    for (const auto& message : port.sent) {
        if (message.type == type) {
            return true;
        }
    }
    return false;
}

} // namespace

int main()
{
    const auto root = fs::temp_directory_path() / "lofibox_zero_pocket_groove_midi_runtime_smoke";
    std::error_code ec{};
    fs::remove_all(root, ec);
    fs::create_directories(root / "xdg-data");
    fs::create_directories(root / "xdg-cache");
    fs::create_directories(root / "xdg-config");
    setEnvPath("XDG_DATA_HOME", root / "xdg-data");
    setEnvPath("XDG_CACHE_HOME", root / "xdg-cache");
    setEnvPath("XDG_CONFIG_HOME", root / "xdg-config");
    setEnvPath("HOME", root / "home");

    auto backend = std::make_shared<FakeAudioBackend>();
    auto midi = std::make_shared<FakeMidiPort>();
    auto services = lofibox::app::withNullRuntimeServices();
    services.playback.audio_backend = backend;
    services.midi.port = midi;

    lofibox::application::AppServiceHost app_host{services};
    lofibox::runtime::RuntimeHost runtime_host{app_host.registry()};
    lofibox::app::AppRuntimeContext app{{}, {}, lofibox::ui::defaultTheme(), app_host, runtime_host.client()};

    app.enterPocketGroove();
    app.update();
    assert(app.snapshot().current_page == lofibox::app::AppPage::PocketGroove);
    assert(midi->open_calls == 1);
    assert(app.pocketGrooveMidiOverlayView().device == "OPEN");

    midi->incoming.push_back(lofibox::midi::MidiMessage{lofibox::midi::MidiMessageType::NoteOn, 10, 38, 100});
    app.update();
    assert(app.pocketGrooveMainView().selectedSlot == 2);
    assert(app.pocketGrooveMainView().footer.find("MIDI NOTE S03") != std::string::npos);

    press(app, lofibox::app::InputKey::F8, "F8");
    press(app, lofibox::app::InputKey::Right, "RIGHT");
    midi->incoming.push_back(lofibox::midi::MidiMessage{lofibox::midi::MidiMessageType::Start});
    midi->incoming.push_back(lofibox::midi::MidiMessage{lofibox::midi::MidiMessageType::Clock});
    midi->incoming.push_back(lofibox::midi::MidiMessage{lofibox::midi::MidiMessageType::Clock});
    app.update();
    assert(app.pocketGrooveMainView().playing);
    assert(backend->play_file_calls > 0);
    assert(app.pocketGrooveMidiOverlayView().sync.rfind("LOCK", 0) == 0);

    midi->incoming.push_back(lofibox::midi::MidiMessage{lofibox::midi::MidiMessageType::Stop});
    app.update();
    assert(!app.pocketGrooveMainView().playing);
    assert(backend->stop_calls > 0);

    press(app, lofibox::app::InputKey::Right, "RIGHT");
    assert(app.pocketGrooveMidiOverlayView().clock == "send");
    press(app, lofibox::app::InputKey::Backspace, "BACK");
    press(app, lofibox::app::InputKey::F2, "F2");
    app.update();
    assert(sentType(*midi, lofibox::midi::MidiMessageType::Start));
    assert(sentType(*midi, lofibox::midi::MidiMessageType::Clock));

    press(app, lofibox::app::InputKey::F2, "F2");
    app.update();
    assert(sentType(*midi, lofibox::midi::MidiMessageType::Stop));

    fs::remove_all(root, ec);
    return 0;
}
