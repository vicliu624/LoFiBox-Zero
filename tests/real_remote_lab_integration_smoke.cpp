// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "app/runtime_services.h"
#include "platform/host/runtime_services_factory.h"

namespace {

struct LabCase {
    lofibox::app::RemoteServerKind kind;
    std::string id;
    std::string name;
    std::string base_url;
    std::string username;
    std::string password;
};

std::string envOrEmpty(const char* name)
{
    const char* value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string{value};
}

lofibox::app::RemoteServerProfile profileFor(const LabCase& test)
{
    lofibox::app::RemoteServerProfile profile{};
    profile.kind = test.kind;
    profile.id = test.id;
    profile.name = test.name;
    profile.base_url = test.base_url;
    profile.username = test.username;
    profile.password = test.password;
    return profile;
}

bool exerciseCase(
    const LabCase& test,
    const lofibox::app::RuntimeServices& services)
{
    const auto profile = profileFor(test);
    const auto session = services.remote.remote_source_provider->probe(profile);
    if (!session.available) {
        std::cerr << "Expected " << test.name << " probe to succeed: " << session.message << "\n";
        return false;
    }

    const auto searched = services.remote.remote_catalog_provider->searchTracks(profile, session, "", 5);
    if (searched.empty()) {
        std::cerr << "Expected " << test.name << " search to return at least one track.\n";
        return false;
    }

    const auto recent = services.remote.remote_catalog_provider->recentTracks(profile, session, 5);
    if (recent.empty()) {
        std::cerr << "Expected " << test.name << " recent tracks to return at least one track.\n";
        return false;
    }

    const auto library = services.remote.remote_catalog_provider->libraryTracks(profile, session, 5);
    if (library.empty()) {
        std::cerr << "Expected " << test.name << " library tracks to return at least one track.\n";
        return false;
    }

    const auto browsed = services.remote.remote_catalog_provider->browse(profile, session, lofibox::app::RemoteCatalogNode{}, 5);
    if (browsed.empty()) {
        std::cerr << "Expected " << test.name << " browse root to return visible nodes.\n";
        return false;
    }

    const auto stream = services.remote.remote_stream_resolver->resolveTrack(profile, session, library.front());
    if (!stream || stream->url.empty() || !stream->diagnostics.connected) {
        std::cerr << "Expected " << test.name << " stream resolver to return a connected URL.\n";
        return false;
    }

    if (library.front().source_label.empty() || library.front().source_id.empty()) {
        std::cerr << "Expected " << test.name << " tracks to carry normalized source attribution.\n";
        return false;
    }

    return true;
}

} // namespace

int main()
{
    const auto lab_url = envOrEmpty("LOFIBOX_REMOTE_LAB_URL");
    if (lab_url.empty()) {
        std::cout << "Remote lab integration environment is not configured; skipping.\n";
        return 0;
    }

    auto services = lofibox::platform::host::createHostRuntimeServices();
    if (!services.remote.remote_source_provider
        || !services.remote.remote_catalog_provider
        || !services.remote.remote_stream_resolver
        || !services.remote.remote_source_provider->available()) {
        std::cerr << "Remote media runtime services are unavailable.\n";
        return 1;
    }

    const std::vector<LabCase> cases{
        {lofibox::app::RemoteServerKind::OpenSubsonic,
         "lab-opensubsonic",
         "OpenSubsonic Lab",
         lab_url + "/opensubsonic",
         "lab",
         "lab"},
        {lofibox::app::RemoteServerKind::Navidrome,
         "lab-navidrome",
         "Navidrome Lab",
         lab_url + "/navidrome",
         "lab",
         "lab"},
        {lofibox::app::RemoteServerKind::PlaylistManifest,
         "lab-m3u",
         "M3U Lab",
         lab_url + "/playlist/lofibox.m3u",
         "",
         ""},
        {lofibox::app::RemoteServerKind::WebDav,
         "lab-webdav",
         "WebDAV Lab",
         lab_url + "/webdav/",
         "",
         ""},
        {lofibox::app::RemoteServerKind::Ftp,
         "lab-ftp",
         "FTP Lab",
         "ftp://192.168.50.48/",
         "",
         ""},
        {lofibox::app::RemoteServerKind::Sftp,
         "lab-sftp",
         "SFTP Lab",
         "sftp://vicliu@192.168.50.48/home/vicliu/lofibox-remote-lab/media/sftp",
         "vicliu",
         ""},
        {lofibox::app::RemoteServerKind::DlnaUpnp,
         "lab-dlna",
         "DLNA Lab",
         lab_url + "/dlna/root.xml",
         "",
         ""},
        {lofibox::app::RemoteServerKind::Smb,
         "lab-smb",
         "SMB Lab",
         "file:///mnt/lofibox-remote-lab-smb",
         "",
         ""},
        {lofibox::app::RemoteServerKind::Nfs,
         "lab-nfs",
         "NFS Lab",
         "file:///mnt/lofibox-remote-lab-nfs",
         "",
         ""},
        {lofibox::app::RemoteServerKind::DirectUrl,
         "lab-direct",
         "Direct URL Lab",
         lab_url + "/media/direct/DIRECT%20Artist/DIRECT%20Album/01%20LoFiBox%20DIRECT%20Track.mp3",
         "",
         ""},
        {lofibox::app::RemoteServerKind::InternetRadio,
         "lab-radio",
         "Radio Lab",
         lab_url + "/media/radio/RADIO%20Artist/RADIO%20Album/01%20LoFiBox%20RADIO%20Track.mp3",
         "",
         ""},
        {lofibox::app::RemoteServerKind::Hls,
         "lab-hls",
         "HLS Lab",
         lab_url + "/hls/lofibox.m3u8",
         "",
         ""},
        {lofibox::app::RemoteServerKind::Dash,
         "lab-dash",
         "DASH Lab",
         lab_url + "/dash/lofibox.mpd",
         "",
         ""},
    };

    for (const auto& test : cases) {
        if (!exerciseCase(test, services)) {
            return 1;
        }
    }

    std::cout << "Remote lab integration smoke passed with " << cases.size() << " source kind(s).\n";
    return 0;
}
