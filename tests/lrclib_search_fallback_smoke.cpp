// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>

#include "app/runtime_services.h"
#include "platform/host/runtime_enrichment_clients.h"
#include "platform/host/runtime_host_internal.h"

int main()
{
    using lofibox::platform::host::runtime_detail::LrclibClient;
    using lofibox::platform::host::runtime_detail::LyricsOvhClient;
    using lofibox::platform::host::runtime_detail::probeConnectivity;

    LrclibClient client{};
    LyricsOvhClient lyrics_ovh{};
    if ((!client.available() && !lyrics_ovh.available()) || !probeConnectivity()) {
        std::cout << "LRCLIB/network unavailable; skipping LRCLIB search fallback smoke.\n";
        return 0;
    }

    if (client.available()) {
        lofibox::app::TrackMetadata metadata{};
        metadata.title = "When We Were Young";
        metadata.artist = "Adele";
        metadata.album = "Wrong Local Folder";
        metadata.duration_seconds = 291;

        const auto lyrics = client.fetch(std::filesystem::path{"Adele-When-We-Were-Young.mp3"}, metadata);
        if (!lyrics.plain && !lyrics.synced) {
            std::cerr << "Expected LRCLIB search fallback to find lyrics when /get metadata is too strict.\n";
            return 1;
        }

        lofibox::app::TrackMetadata cover_metadata{};
        cover_metadata.title = "My Heart Will Go On";
        cover_metadata.artist = std::string("\xE6\xBB\xA1") + "\xE8\x88\x92" + "\xE5\x85\x8B";
        cover_metadata.duration_seconds = 291;

        const auto cover_lyrics = client.fetch(std::filesystem::path{"cover-collaboration-My-Heart-Will-Go-On.flac"}, cover_metadata);
        if (!cover_lyrics.plain && !cover_lyrics.synced) {
            std::cerr << "Expected LRCLIB title-only fallback to find lyrics for cover/collaboration metadata.\n";
            return 1;
        }
    }

    if (lyrics_ovh.available()) {
        lofibox::app::TrackMetadata lyrics_ovh_metadata{};
        lyrics_ovh_metadata.title = "Yellow";
        lyrics_ovh_metadata.artist = "Coldplay";
        const auto lyrics_ovh_lyrics = lyrics_ovh.fetch(std::filesystem::path{"Coldplay-Yellow.mp3"}, lyrics_ovh_metadata);
        if (!lyrics_ovh_lyrics.plain || lyrics_ovh_lyrics.source != "lyrics.ovh") {
            std::cerr << "Expected lyrics.ovh fallback client to fetch plain lyrics.\n";
            return 1;
        }
    }

    return 0;
}
