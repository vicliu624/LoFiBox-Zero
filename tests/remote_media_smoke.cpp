// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <functional>
#include <thread>

#include "app/runtime_services.h"
#include "platform/host/runtime_services_factory.h"
#include "media_fixture_utils.h"

namespace fs = std::filesystem;

int main()
{
#if defined(_WIN32)
    const auto python_path = test_media_fixture::resolveExecutablePath(L"PYTHON_EXECUTABLE", L"python.exe");
#elif defined(__linux__)
    const auto python_path = test_media_fixture::resolveExecutablePath("PYTHON_EXECUTABLE", "python3");
#else
    const auto python_path = std::optional<fs::path>{};
#endif
    if (!python_path.has_value()) {
        std::cout << "python unavailable; skipping remote media smoke.\n";
        return 0;
    }

    const fs::path root = fs::temp_directory_path() / "lofibox_zero_remote_media_smoke";
    std::error_code ec{};
    fs::remove_all(root, ec);
    fs::create_directories(root, ec);
    if (ec) {
        std::cerr << "Failed to create temp directory.\n";
        return 1;
    }
    const int port = 18000 + static_cast<int>((std::chrono::steady_clock::now().time_since_epoch().count() % 1000 + 1000) % 1000);

    const fs::path server_script = root / "server.py";
    {
        std::ofstream output(server_script, std::ios::binary | std::ios::trunc);
        output <<
R"(import json
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import parse_qs, urlparse

class Handler(BaseHTTPRequestHandler):
    def log_message(self, *args): pass
    def _write(self, data):
        payload = json.dumps(data).encode('utf-8')
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def do_POST(self):
        if self.path.endswith('/Users/AuthenticateByName'):
            self._write({'AccessToken':'token-123','User':{'Id':'user-1'}})
            return
        self.send_response(404); self.end_headers()

    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path.endswith('/System/Info/Public'):
            self._write({'ServerName':'TestServer'})
        elif parsed.path.endswith('/Users/user-1/Items'):
            self._write({'Items':[{'Id':'trk1','Name':'Song A','Album':'Album A','AlbumId':'alb1','Artists':['Artist A'],'RunTimeTicks':1200000000}]})
        elif parsed.path.endswith('/Users/user-1/Items/Latest'):
            self._write([{'Id':'trk1','Name':'Song A','Album':'Album A','AlbumId':'alb1','Artists':['Artist A'],'RunTimeTicks':1200000000}])
        elif parsed.path.endswith('/rest/ping.view'):
            self._write({'subsonic-response':{'status':'ok'}})
        elif parsed.path.endswith('/rest/search3.view'):
            self._write({'subsonic-response':{'searchResult3':{'song':[{'id':'trk2','title':'Song B','artist':'Artist B','album':'Album B','parent':'alb2','duration':123}]}}})
        elif parsed.path.endswith('/rest/getAlbumList2.view'):
            self._write({'subsonic-response':{'albumList2':{'album':[{'id':'alb2','name':'Album B'}]}}})
        elif parsed.path.endswith('/rest/getAlbum.view'):
            self._write({'subsonic-response':{'album':{'song':[{'id':'trk2','title':'Song B','artist':'Artist B','album':'Album B','parent':'alb2','duration':123}]}}})
        else:
            self.send_response(404); self.end_headers()

server = HTTPServer(('127.0.0.1', )" << port << R"(), Handler)
server.serve_forever()
)";
    }

    const std::vector<std::string> server_args{server_script.string()};
    // Keep it simple: start python server detached via OS-specific helper script process.
#if defined(_WIN32)
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    std::wstring cmd = L"python \"" + server_script.wstring() + L"\"";
    if (!CreateProcessW(python_path->wstring().c_str(), cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process)) {
        std::cerr << "Failed to start mock server.\n";
        fs::remove_all(root, ec);
        return 1;
    }
    CloseHandle(process.hThread);
    const auto cleanup = [&]() {
        TerminateProcess(process.hProcess, 0);
        WaitForSingleObject(process.hProcess, 1000);
        CloseHandle(process.hProcess);
        std::error_code cleanup_ec{};
        fs::remove_all(root, cleanup_ec);
    };
#elif defined(__linux__)
    pid_t pid = fork();
    if (pid == 0) {
        execl(python_path->string().c_str(), python_path->string().c_str(), server_script.string().c_str(), nullptr);
        _exit(127);
    } else if (pid < 0) {
        std::cerr << "Failed to start mock server.\n";
        fs::remove_all(root, ec);
        return 1;
    }
    const auto cleanup = [&]() {
        kill(pid, SIGTERM);
        waitpid(pid, nullptr, 0);
        std::error_code cleanup_ec{};
        fs::remove_all(root, cleanup_ec);
    };
#endif
    struct ScopeExit {
        std::function<void()> fn{};
        ~ScopeExit()
        {
            if (fn) {
                fn();
            }
        }
    } scope_exit{cleanup};

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    auto services = lofibox::platform::host::createHostRuntimeServices();
    if (!services.remote_source_provider->available()
        || !services.remote_catalog_provider->available()
        || !services.remote_stream_resolver->available()) {
        std::cerr << "Expected remote media capabilities to be available.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const auto jellyfin_profile = lofibox::app::RemoteServerProfile{
        lofibox::app::RemoteServerKind::Jellyfin, "jf", "Jellyfin", "http://127.0.0.1:" + std::to_string(port), "user", "pass", ""};
    const auto jf_session = services.remote_source_provider->probe(jellyfin_profile);
    if (!jf_session.available || jf_session.access_token.empty()) {
        std::cerr << "Expected Jellyfin probe to succeed.\n";
        return 1;
    }
    const auto jf_tracks = services.remote_catalog_provider->searchTracks(jellyfin_profile, jf_session, "Song", 5);
    if (jf_tracks.empty() || jf_tracks.front().id != "trk1") {
        std::cerr << "Expected Jellyfin search to return a track.\n";
        return 1;
    }
    const auto jf_stream = services.remote_stream_resolver->resolveTrack(jellyfin_profile, jf_session, jf_tracks.front());
    if (!jf_stream || jf_stream->url.find("/Audio/trk1/stream") == std::string::npos) {
        std::cerr << "Expected Jellyfin resolver to build stream URL.\n";
        return 1;
    }

    const auto subsonic_profile = lofibox::app::RemoteServerProfile{
        lofibox::app::RemoteServerKind::OpenSubsonic, "sub", "Navidrome", "http://127.0.0.1:" + std::to_string(port), "user", "pass", ""};
    const auto sub_session = services.remote_source_provider->probe(subsonic_profile);
    if (!sub_session.available) {
        std::cerr << "Expected OpenSubsonic probe to succeed.\n";
        return 1;
    }
    const auto sub_tracks = services.remote_catalog_provider->searchTracks(subsonic_profile, sub_session, "Song", 5);
    if (sub_tracks.empty() || sub_tracks.front().id != "trk2") {
        std::cerr << "Expected OpenSubsonic search to return a track.\n";
        return 1;
    }
    const auto sub_stream = services.remote_stream_resolver->resolveTrack(subsonic_profile, sub_session, sub_tracks.front());
    if (!sub_stream || sub_stream->url.find("/rest/stream.view") == std::string::npos) {
        std::cerr << "Expected OpenSubsonic resolver to build stream URL.\n";
        return 1;
    }

    const auto emby_profile = lofibox::app::RemoteServerProfile{
        lofibox::app::RemoteServerKind::Emby, "emb", "Emby", "http://127.0.0.1:" + std::to_string(port), "user", "pass", ""};
    const auto emby_session = services.remote_source_provider->probe(emby_profile);
    if (!emby_session.available || emby_session.access_token.empty()) {
        std::cerr << "Expected Emby probe to succeed.\n";
        return 1;
    }
    const auto emby_tracks = services.remote_catalog_provider->searchTracks(emby_profile, emby_session, "Song", 5);
    if (emby_tracks.empty() || emby_tracks.front().id != "trk1") {
        std::cerr << "Expected Emby search to return a track.\n";
        return 1;
    }
    const auto emby_stream = services.remote_stream_resolver->resolveTrack(emby_profile, emby_session, emby_tracks.front());
    if (!emby_stream || emby_stream->url.find("/Audio/trk1/stream.mp3") == std::string::npos) {
        std::cerr << "Expected Emby resolver to build stream URL.\n";
        return 1;
    }

    return 0;
}
