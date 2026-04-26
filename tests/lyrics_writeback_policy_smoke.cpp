// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/lyrics_pipeline_components.h"

#include <cassert>

int main()
{
    lofibox::platform::host::runtime_detail::LyricsWritebackPolicy policy{};
    lofibox::app::TrackLyrics lyrics{};
    lyrics.source = "LRCLIB";
    lyrics.synced = "[00:01.00]hello";

    lofibox::app::TrackIdentity trusted{};
    trusted.found = true;
    trusted.confidence = 0.90;
    assert(policy.shouldWrite(lyrics, trusted));

    lofibox::app::TrackIdentity weak{};
    weak.found = true;
    weak.confidence = 0.40;
    assert(!policy.shouldWrite(lyrics, weak));

    lyrics.source = "lyrics.ovh";
    trusted.confidence = 0.95;
    assert(!policy.shouldWrite(lyrics, trusted));
    return 0;
}
