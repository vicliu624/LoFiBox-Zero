// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "app/runtime_services.h"
#include "platform/host/runtime_enrichment_client_helpers.h"
#include "platform/host/runtime_enrichment_clients.h"
#include "platform/host/runtime_host_internal.h"

namespace {

bool networkSmokeEnabled()
{
    const char* value = std::getenv("LOFIBOX_RUN_NETWORK_LYRICS_SMOKE");
    return value != nullptr && std::string(value) == "1";
}

bool verifyOfflineLyricsParsers()
{
    using lofibox::platform::host::runtime_detail::LrclibLookupSeed;
    using lofibox::platform::host::runtime_detail::LyricsOvhSuggestion;
    using lofibox::platform::host::runtime_detail::bestLrclibLyricsFromJson;
    using lofibox::platform::host::runtime_detail::lyricsOvhLyricsFromJson;
    using lofibox::platform::host::runtime_detail::lyricsOvhSuggestionsFromJson;

    const std::vector<LrclibLookupSeed> seeds{
        LrclibLookupSeed{"Yellow", "Coldplay", "", 266, false},
    };
    const auto lrclib_lyrics = bestLrclibLyricsFromJson(
        R"([{"trackName":"Yellow","artistName":"Coldplay","duration":266,"plainLyrics":"Look at the stars","syncedLyrics":"[00:01.00]Look at the stars"}])",
        seeds);
    if (!lrclib_lyrics.plain || !lrclib_lyrics.synced || lrclib_lyrics.source != "LRCLIB") {
        std::cerr << "Expected LRCLIB parser to choose a matching offline lyric payload.\n";
        return false;
    }

    const auto lyrics_ovh_lyrics = lyricsOvhLyricsFromJson(R"({"lyrics":"Look at the stars\nLook how they shine"})");
    if (!lyrics_ovh_lyrics.plain || lyrics_ovh_lyrics.source != "lyrics.ovh") {
        std::cerr << "Expected lyrics.ovh parser to read plain lyrics from an offline payload.\n";
        return false;
    }

    const auto suggestions = lyricsOvhSuggestionsFromJson(
        R"({"data":[{"title":"Yellow","title_short":"Yellow","artist":{"name":"Coldplay"}}]})",
        seeds);
    if (suggestions.size() != 1 || suggestions.front().title != "Yellow" || suggestions.front().artist != "Coldplay") {
        std::cerr << "Expected lyrics.ovh suggestion parser to keep matching offline suggestions.\n";
        return false;
    }

    return true;
}

} // namespace

int main()
{
    using lofibox::platform::host::runtime_detail::LrclibClient;
    using lofibox::platform::host::runtime_detail::LyricsOvhClient;
    using lofibox::platform::host::runtime_detail::probeConnectivity;

    if (!verifyOfflineLyricsParsers()) {
        return 1;
    }

    if (!networkSmokeEnabled()) {
        std::cout << "Network lyrics smoke disabled; offline lyric parser coverage passed.\n";
        return 0;
    }

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
