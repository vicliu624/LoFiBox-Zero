// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include <memory>
#include "core/session_timer.h"
#include "app/runtime_services.h"
#include "application/app_service_host.h"
#include "runtime/runtime_session_facade.h"
#include "runtime/runtime_envelope_serializer.h"

namespace {
class Audio final : public lofibox::app::AudioPlaybackBackend {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "timer-test"; }
    bool playFile(const std::filesystem::path&, double) override { playing = true; return true; }
    void stop() override { playing = false; ++stops; }
    bool isPlaying() override { return playing; }
    bool isFinished() override { return false; }
    bool playing{true};
    int stops{};
};
}
int main()
{
    using namespace std::chrono_literals;
    using namespace lofibox::runtime;
    using lofibox::core::parseTimerDuration;
    assert(parseTimerDuration("OFF") == 0s);
    assert(parseTimerDuration("15M") == 900s && parseTimerDuration("24H") == 86400s);
    for (const auto text : {"", "ON", "0S", "-1M", "1m", "25H", "1.5M", "999999999M", "1Mgarbage"})
        assert(!parseTimerDuration(text));
    SettingsRuntimeState state{};
    SettingsRuntime settings{state};
    const auto now = std::chrono::steady_clock::now();
    assert(settings.applyLive({}, {}, "15M", "30S", now));
    assert(!settings.sleepTimerExpired(now + 899s));
    assert(settings.sleepTimerExpired(now + 900s));
    assert(settings.screenTimeout() == 30s);
    assert(settings.applyLive({}, {}, {}, "1M", now + 500s));
    assert(settings.sleepTimerExpired(now + 900s)); // screen change never resets exit
    assert(!settings.applyLive("BAD", {}, "2M", "broken", now));
    assert(settings.snapshot(0).output_mode == "DEFAULT"); // update is atomic
    assert(settings.snapshot(0).sleep_timer == "15M");
    assert(settings.applyLive({}, {}, "OFF", {}, now + 901s));
    assert(!settings.sleepTimerExpired(now + 90000s));
    assert(settings.screenTimeout() == 60s);

    RuntimeCommandRequest request{RuntimeCommand{RuntimeCommandKind::SettingsApplyLive,
        RuntimeCommandPayload::settingsApplyLive({}, {}, "30M", "2M")}};
    const auto parsed = parseRuntimeCommandRequest(serializeRuntimeCommandRequest(request));
    assert(parsed && parsed->command.payload.get<SettingsApplyLivePayload>()->screen_timeout == "2M");
    RuntimeQueryResponse response{};
    response.snapshot.settings = settings.snapshot(42);
    const auto roundtrip = parseRuntimeQueryResponse(serializeRuntimeQueryResponse(response));
    assert(roundtrip && roundtrip->snapshot.settings.screen_timeout == "1M");
    const auto legacy = parseRuntimeCommandRequest(
        "{\"kind\":\"SettingsApplyLive\",\"payload\":\"SettingsApplyLive\",\"sleep_timer\":\"15M\"}");
    assert(legacy && legacy->command.payload.get<SettingsApplyLivePayload>());
    assert(legacy->command.payload.get<SettingsApplyLivePayload>()->screen_timeout.empty());

    auto services = lofibox::app::withNullRuntimeServices();
    auto audio = std::make_shared<Audio>();
    services.playback.audio_backend = audio;
    lofibox::application::AppServiceHost host{services};
    EqRuntimeState eq{};
    SettingsRuntimeState timed_state{};
    RuntimeSessionFacade session{host.registry(), eq, timed_state};
    assert(session.settings().applyLive({}, {}, "1S", {}, now - 2s));
    const auto stops_before = audio->stops;
    session.tick(0.033);
    assert(session.settings().shutdownRequested());
    assert(!audio->playing && audio->stops == stops_before + 1);
    session.tick(0.033);
    assert(audio->stops == stops_before + 1); // stop/cleanup exactly once
}
