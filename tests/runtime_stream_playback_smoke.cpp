// SPDX-License-Identifier: GPL-3.0-or-later
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include "application/app_service_host.h"
#include "runtime/runtime_host.h"

namespace {
using namespace lofibox;
class Backend final : public app::AudioPlaybackBackend {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "STREAM-TEST"; }
    bool playFile(const std::filesystem::path&, double seconds) override
    { kind="file";position=seconds;current=app::AudioPlaybackState::Playing;++starts;return true; }
    bool playUri(const std::string& input, double seconds) override
    { kind="uri";uri=input;position=seconds;++starts;current=fail?app::AudioPlaybackState::Failed:app::AudioPlaybackState::Playing;return !fail; }
    void pause() override { current=app::AudioPlaybackState::Paused; }
    void resume() override { current=app::AudioPlaybackState::Playing; }
    void stop() override { current=app::AudioPlaybackState::Idle; }
    bool isPlaying() override { return current==app::AudioPlaybackState::Playing; }
    bool isFinished() override { return current==app::AudioPlaybackState::Finished; }
    app::AudioPlaybackState state() override { return current; }
    std::optional<double> positionSeconds() const override { return position; }
    app::AudioPlaybackState current{app::AudioPlaybackState::Idle};
    std::string kind,uri;
    double position{0};
    int starts{0};
    bool fail{false};
};
void expect(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
}

int main()
{
    using namespace lofibox;
    using runtime::RuntimeCommandKind;
    try {
        auto services=app::withNullRuntimeServices();
        auto backend=std::make_shared<Backend>();services.playback.audio_backend=backend;
        application::AppServiceHost app{services};
        runtime::RuntimeHost host{app.registry()};
        auto dispatch=[&](RuntimeCommandKind kind,runtime::RuntimeCommandPayload payload={}) {
            return host.client().dispatch({kind,std::move(payload),runtime::CommandOrigin::DirectTest});
        };
        auto snapshot=[&] {return host.client().query({runtime::RuntimeQueryKind::PlaybackSnapshot}).playback;};
        app::ResolvedRemoteStream stream{};
        stream.url="https://example.test/music.mp3?private=value";
        stream.diagnostics.resolved_url_redacted="https://example.test/music.mp3";
        app::RemoteTrack track{};track.title="Stream";track.duration_seconds=10;
        auto open=[&] {
            expect(dispatch(RuntimeCommandKind::RemoteStartActiveStream,
                runtime::RuntimeCommandPayload::remoteResolvedStream(stream,track,"DIRECT",{})).applied,"stream start failed");
        };
        open();
        expect(snapshot().seekable,"on-demand stream was not seekable");
        expect(dispatch(RuntimeCommandKind::PlaybackSeek,runtime::RuntimeCommandPayload::seek(4)).applied,"on-demand stream seek was rejected");
        expect(backend->kind=="uri" && backend->position==4 && backend->uri==stream.url,"stream seek lost input or position");
        const auto starts=backend->starts;
        for (double value:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),1e308}) {
            expect(!dispatch(RuntimeCommandKind::PlaybackSeek,runtime::RuntimeCommandPayload::seek(value)).applied,"non-finite seek was accepted");
        }
        expect(backend->starts==starts,"invalid seek replaced playback");
        dispatch(RuntimeCommandKind::PlaybackPause);dispatch(RuntimeCommandKind::PlaybackResume);
        expect(backend->starts==starts && snapshot().audio_active,"paused stream was unnecessarily restarted");
        dispatch(RuntimeCommandKind::PlaybackStop);
        expect(dispatch(RuntimeCommandKind::PlaybackResume).applied && backend->position==0,"stopped stream could not restart at zero");
        backend->position=10;backend->current=app::AudioPlaybackState::Finished;host.tick(.1);
        expect(!snapshot().audio_active,"finished stream remained active");
        expect(dispatch(RuntimeCommandKind::PlaybackToggle).applied && backend->position==0,"finished stream could not replay from zero");
        dispatch(RuntimeCommandKind::PlaybackSetRepeatOne,runtime::RuntimeCommandPayload::enabled(true));
        backend->position=10;backend->current=app::AudioPlaybackState::Finished;host.tick(.1);
        expect(snapshot().audio_active && backend->position==0,"single direct stream repeat did not restart");
        dispatch(RuntimeCommandKind::PlaybackSetRepeatAll,runtime::RuntimeCommandPayload::enabled(false));
        expect(!snapshot().repeat_one && !snapshot().repeat_all,"repeat off retained an active repeat-one mode");
        backend->fail=true;
        expect(!dispatch(RuntimeCommandKind::PlaybackSeek,runtime::RuntimeCommandPayload::seek(6)).applied,"backend failure was reported successful");
        backend->fail=false;
        expect(dispatch(RuntimeCommandKind::PlaybackResume).applied && backend->position==6,"failed stream seek could not retry");
        for (int mode=0;mode<3;++mode) {
            stream.seekable=mode!=0;stream.diagnostics.seekable=mode!=1;stream.diagnostics.live=mode==2;
            open();
            expect(!snapshot().seekable,"unseekable/live stream advertised a seek control");
            const auto before=backend->starts;
            expect(!dispatch(RuntimeCommandKind::PlaybackSeek,runtime::RuntimeCommandPayload::seek(3)).applied,"unseekable/live stream accepted seeking");
            expect(backend->starts==before,"rejected seek replaced unseekable playback");
            dispatch(RuntimeCommandKind::PlaybackStop);
            expect(dispatch(RuntimeCommandKind::PlaybackResume).applied && backend->position==0,"unseekable stream could not reconnect from start");
        }
        stream.seekable=true;stream.diagnostics.seekable=true;stream.diagnostics.live=false;
        auto& model=app.controllers().library.mutableModel();
        app::TrackRecord remote{};remote.id=77;remote.remote=true;remote.remote_profile_id="profile";remote.remote_track_id="item";remote.duration_seconds=10;
        model.tracks.push_back(remote);
        app::RemoteServerProfile profile{};profile.id="profile";
        expect(dispatch(RuntimeCommandKind::RemoteStartActiveStream,
            runtime::RuntimeCommandPayload::remoteResolvedLibraryTrack(77,profile,track,stream,"REMOTE",false,{})).applied,"remote library stream start failed");
        expect(dispatch(RuntimeCommandKind::PlaybackSeek,runtime::RuntimeCommandPayload::seek(7)).applied && backend->position==7,"remote library stream could not seek");
        backend->position=10;backend->current=app::AudioPlaybackState::Finished;host.tick(.1);
        expect(dispatch(RuntimeCommandKind::PlaybackResume).applied && backend->position==0,"remote library EOF replay tried to resolve a missing profile");
        backend->position=10;backend->current=app::AudioPlaybackState::Finished;host.tick(.1);
        expect(dispatch(RuntimeCommandKind::PlaybackPlay).applied && backend->position==0,"generic play could not replay a remote library stream");
        model.tracks.push_back(app::TrackRecord{61,"local.mp3","Local","Artist","Album"});
        app.controllers().library.setSongsContextAll();
        expect(dispatch(RuntimeCommandKind::PlaybackStartTrack,runtime::RuntimeCommandPayload::startTrack(61)).applied,"local transition failed");
        dispatch(RuntimeCommandKind::PlaybackStop);
        expect(dispatch(RuntimeCommandKind::PlaybackResume).applied && backend->kind=="file","local resume reused an old stream URI");
        expect(snapshot().seekable && !snapshot().live,"local transition retained stream capabilities");
        std::cout<<"Stream seek, capability guards, pause/resume, stop/EOF replay, retry and local transitions passed\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
