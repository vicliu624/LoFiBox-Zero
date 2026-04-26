// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "platform/host/runtime_host_internal.h"

namespace fs = std::filesystem;
namespace runtime_detail = lofibox::platform::host::runtime_detail;

namespace {

bool containsGuess(const std::vector<runtime_detail::FilenameMetadataGuess>& guesses, const std::string& artist, const std::string& title)
{
    for (const auto& guess : guesses) {
        if (guess.artist == artist && guess.title == title) {
            return true;
        }
    }
    return false;
}

std::string hexString(const std::string& value)
{
    std::ostringstream output{};
    output << std::hex << std::setfill('0');
    for (const unsigned char ch : value) {
        output << std::setw(2) << static_cast<int>(ch);
    }
    return output.str();
}

fs::path utf8Path(const std::string& value)
{
    std::u8string utf8{};
    utf8.reserve(value.size());
    for (const unsigned char ch : value) {
        utf8.push_back(static_cast<char8_t>(ch));
    }
    return fs::path{utf8};
}

} // namespace

int main()
{
    const auto john = runtime_detail::fallbackMetadataGuessesFromPath(fs::path{"John-Lennon-Stand-By-Me.mp3"});
    if (!containsGuess(john, "John Lennon", "Stand By Me")) {
        std::cerr << "Expected compact hyphen parser to include John Lennon / Stand By Me.\n";
        return 1;
    }

    const auto xjapan = runtime_detail::fallbackMetadataGuessesFromPath(fs::path{"X-JAPAN-Endless-Rain.mp3"});
    if (!containsGuess(xjapan, "X JAPAN", "Endless Rain")) {
        std::cerr << "Expected compact hyphen parser to include X JAPAN / Endless Rain.\n";
        return 1;
    }

    const auto eminem = runtime_detail::fallbackMetadataGuessesFromPath(fs::path{"Eminem-Better-In-Time-_Remix_.mp3"});
    if (!containsGuess(eminem, "Eminem", "Better In Time Remix")) {
        std::cerr << "Expected compact hyphen parser to include Eminem / Better In Time Remix.\n";
        return 1;
    }
    if (containsGuess(eminem, "Eminem Better In Time", "Remix")) {
        std::cerr << "Expected compact hyphen parser to avoid over-broad one-word title splits.\n";
        return 1;
    }
    lofibox::app::TrackMetadata bad_eminem_metadata{};
    bad_eminem_metadata.title = "Moments in Time (remix)";
    bad_eminem_metadata.artist = "McGee Pragnell Band";
    if (runtime_detail::metadataCompatibleWithFilename(fs::path{"Eminem-Better-In-Time-_Remix_.mp3"}, bad_eminem_metadata)) {
        std::cerr << "Expected wrong embedded metadata to conflict with Eminem filename.\n";
        return 1;
    }

    const auto suede = runtime_detail::fallbackMetadataGuessesFromPath(fs::path{"Suede-Beautiful-Ones-_Single-Version_.mp3"});
    if (!containsGuess(suede, "Suede", "Beautiful Ones Single Version")) {
        std::cerr << "Expected compact hyphen parser to include Suede / Beautiful Ones Single Version.\n";
        return 1;
    }
    if (containsGuess(suede, "Suede Beautiful Ones Single", "Version")) {
        std::cerr << "Expected compact hyphen parser to avoid over-broad version-only title splits.\n";
        return 1;
    }
    lofibox::app::TrackMetadata bad_suede_metadata{};
    bad_suede_metadata.title = "The Only Ones (single version)";
    bad_suede_metadata.artist = "Reamonn";
    if (runtime_detail::metadataCompatibleWithFilename(fs::path{"Suede-Beautiful-Ones-_Single-Version_.mp3"}, bad_suede_metadata)) {
        std::cerr << "Expected wrong embedded metadata to conflict with Suede filename.\n";
        return 1;
    }
    const std::string akb_title =
        "\xE5\x88\xB6\xE6\x9C\x8D\xE3\x81\xAE\xE7\xBE\xBD\xE6\xA0\xB9";
    lofibox::app::TrackMetadata akb_metadata{};
    akb_metadata.title = akb_title;
    akb_metadata.artist = "Team 8";
    const auto akb_path = utf8Path(std::string{"AKB48 - "} + akb_title + "(Team 8).ogg");
    if (!runtime_detail::metadataCompatibleWithFilename(akb_path, akb_metadata)) {
        std::cerr << "Expected exact title match to remain compatible even when performer differs from filename artist.\n";
        return 1;
    }

    const auto spaced = runtime_detail::fallbackMetadataGuessesFromPath(fs::path{"Adele - When We Were Young.ogg"});
    if (spaced.empty() || spaced.front().artist != "Adele" || spaced.front().title != "When We Were Young") {
        std::cerr << "Expected spaced separator parser to preserve the canonical artist/title split.\n";
        return 1;
    }

    const std::string mojibake_name =
        "AKB48 - "
        "\xC3\xA5" "\xCB\x86" "\xC2\xB6"
        "\xC3\xA6" "\xC5\x93" "\xC2\x8D"
        "\xC3\xA3" "\xC2\x81" "\xC2\xAE"
        "\xC3\xA7" "\xC2\xBE" "\xC2\xBD"
        "\xC3\xA6" "\xC2\xA0" "\xC2\xB9"
        "(Team 8).ogg";
    const auto mojibake = runtime_detail::fallbackMetadataGuessesFromPath(utf8Path(mojibake_name));
    const std::string recovered_akb_title = akb_title + "(Team 8)";
    if (mojibake.empty() || mojibake.front().artist != "AKB48" || mojibake.front().title != recovered_akb_title) {
        std::cerr << "Expected mojibake filename parser to recover UTF-8 artist/title keywords.\n";
        if (!mojibake.empty()) {
            std::cerr << "Actual artist: " << mojibake.front().artist << "\n";
            std::cerr << "Actual title hex: " << hexString(mojibake.front().title) << "\n";
            std::cerr << "Expected title hex: " << hexString(recovered_akb_title) << "\n";
        }
        return 1;
    }

    const auto flat_music = runtime_detail::fallbackMetadataGuessesFromPath(fs::path{"C:/Users/VicLi/Music/AKB48 - Uniform.ogg"});
    bool has_bogus_user_artist = false;
    for (const auto& guess : flat_music) {
        if (guess.artist == "VicLi") {
            has_bogus_user_artist = true;
        }
    }
    if (has_bogus_user_artist) {
        std::cerr << "Expected flat Music root fallback not to use the OS user name as artist.\n";
        return 1;
    }

    return 0;
}
