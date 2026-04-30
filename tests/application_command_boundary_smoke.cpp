// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "app/app_controller_set.h"
#include "app/remote_profile_store.h"
#include "app/runtime_services.h"
#include "application/app_service_registry.h"
#include "security/credential_policy.h"

namespace {

class FakeAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "FAKE"; }
    bool playFile(const std::filesystem::path&, double) override
    {
        playing = true;
        paused = false;
        return true;
    }
    bool playUri(const std::string&, double) override
    {
        playing = true;
        paused = false;
        return true;
    }
    void stop() override
    {
        playing = false;
        paused = false;
    }
    void pause() override { paused = true; }
    void resume() override
    {
        playing = true;
        paused = false;
    }
    [[nodiscard]] bool isPlaying() override { return playing && !paused; }
    [[nodiscard]] bool isFinished() override { return false; }

private:
    bool playing{false};
    bool paused{false};
};

class MemoryRemoteProfileStore final : public lofibox::app::RemoteProfileStore {
public:
    [[nodiscard]] std::vector<lofibox::app::RemoteServerProfile> loadProfiles() const override
    {
        return loaded;
    }

    bool saveProfiles(const std::vector<lofibox::app::RemoteServerProfile>& profiles) const override
    {
        saved = profiles;
        ++save_profiles_calls;
        return true;
    }

    bool saveCredentials(const lofibox::app::RemoteServerProfile& profile) const override
    {
        saved_credentials = profile;
        ++save_credentials_calls;
        return true;
    }

    bool deleteCredentials(const lofibox::security::CredentialRef& credential_ref) const override
    {
        deleted_credential_ref = credential_ref.id;
        ++delete_credentials_calls;
        return true;
    }

    std::vector<lofibox::app::RemoteServerProfile> loaded{};
    mutable std::vector<lofibox::app::RemoteServerProfile> saved{};
    mutable lofibox::app::RemoteServerProfile saved_credentials{};
    mutable std::string deleted_credential_ref{};
    mutable int save_profiles_calls{0};
    mutable int save_credentials_calls{0};
    mutable int delete_credentials_calls{0};
};

class ProbeRemoteSourceProvider final : public lofibox::app::RemoteSourceProvider {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "PROBE"; }

    [[nodiscard]] lofibox::app::RemoteSourceSession probe(const lofibox::app::RemoteServerProfile& profile) const override
    {
        lofibox::app::RemoteSourceSession session{};
        session.available = !profile.base_url.empty();
        session.server_name = profile.name;
        session.user_id = profile.username;
        session.message = session.available ? "OK" : "MISSING URL";
        return session;
    }
};

} // namespace

int main()
{
    auto runtime = lofibox::app::withNullRuntimeServices();
    auto profile_store = std::make_shared<MemoryRemoteProfileStore>();
    runtime.playback.audio_backend = std::make_shared<FakeAudioBackend>();
    runtime.remote.remote_profile_store = profile_store;
    runtime.remote.remote_source_provider = std::make_shared<ProbeRemoteSourceProvider>();

    lofibox::app::AppControllerSet controllers{};
    controllers.bindServices(runtime);
    lofibox::application::AppServiceRegistry registry{controllers, runtime};

    auto& model = controllers.library.mutableModel();
    model.tracks.push_back(lofibox::app::TrackRecord{42, std::filesystem::path{"song.flac"}, "Song", "Artist", "Album"});
    registry.libraryMutations().setSongsContextAll();

    const auto song_ids = registry.libraryQueries().trackIdsForCurrentSongs();
    if (song_ids.size() != 1 || song_ids.front() != 42) {
        std::cerr << "Expected LibraryQueryService to read the song context established by LibraryMutationService.\n";
        return 1;
    }

    const auto open_result = registry.libraryOpenActions().openSelectedListItem(lofibox::app::AppPage::Songs, 0);
    if (open_result.kind != lofibox::app::LibraryOpenResult::Kind::StartTrack || open_result.track_id != 42) {
        std::cerr << "Expected LibraryOpenActionService to return an explicit product track reference.\n";
        return 1;
    }

    if (!registry.playbackCommands().startTrack(open_result.track_id)) {
        std::cerr << "Expected PlaybackCommandService to start a Library track without exposing PlaybackController.\n";
        return 1;
    }

    const auto status = registry.playbackStatus().snapshot();
    if (status.current_track_id != 42 || status.title != "Song" || status.source_label != "LOCAL") {
        std::cerr << "Expected PlaybackStatusQueryService to expose structured current playback facts.\n";
        return 1;
    }

    auto profiles = registry.sourceProfiles().loadProfiles();
    const auto profile_index = registry.sourceProfiles().ensureProfileForKind(profiles, lofibox::app::RemoteServerKind::Emby);
    auto& profile = profiles.at(profile_index);
    if (profile.id.empty() || registry.sourceProfiles().readiness(profile) != "NEEDS URL") {
        std::cerr << "Expected SourceProfileCommandService to create a stable profile shell.\n";
        return 1;
    }

    (void)registry.sourceProfiles().updateTextField(
        profile,
        lofibox::application::SourceProfileTextField::Address,
        "http://example.test:8096",
        profiles.size());
    (void)registry.sourceProfiles().updateTextField(
        profile,
        lofibox::application::SourceProfileTextField::Username,
        "vicliu",
        profiles.size());
    const auto credential_result = registry.credentials().setSecret(
        profile,
        lofibox::application::CredentialSecretPatch{"", "secret", ""});

    if (!credential_result.accepted || profile_store->save_credentials_calls != 1 || profile.credential_ref.id.empty()) {
        std::cerr << "Expected credential edits to persist through CredentialCommandService with a credential ref.\n";
        return 1;
    }
    if (registry.sourceProfiles().readiness(profile) != "READY") {
        std::cerr << "Expected completed Emby profile to become READY.\n";
        return 1;
    }

    const auto probe = registry.sourceProfiles().probe(profile, profiles.size());
    if (!probe.command.accepted || !probe.session.available || probe.session.user_id != "vicliu") {
        std::cerr << "Expected SourceProfileCommandService probe to return structured availability facts.\n";
        return 1;
    }

    if (!registry.sourceProfiles().persistProfiles(profiles) || profile_store->saved.empty()) {
        std::cerr << "Expected SourceProfileCommandService to persist profile lists.\n";
        return 1;
    }

    return 0;
}
