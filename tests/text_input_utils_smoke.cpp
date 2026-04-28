// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/text_input_utils.h"

#include <cassert>
#include <string>

int main()
{
    const std::string sea = "\xE6\xB5\xB7";
    const std::string no = "\xE3\x81\xAE";
    const std::string song_ko = "\xEB\x85\xB8\xEB\x9E\x98";
    const std::string tokyo_jazz = "\xE6\x9D\xB1\xE4\xBA\xACJazz";

    std::string query{};
    assert(lofibox::app::appendUtf8Bounded(query, sea, 8));
    assert(lofibox::app::appendUtf8Bounded(query, no, 8));
    assert(!lofibox::app::appendUtf8Bounded(query, song_ko, 8));
    assert(query == sea + no);

    assert(lofibox::app::popLastUtf8Codepoint(query));
    assert(query == sea);
    assert(lofibox::app::popLastUtf8Codepoint(query));
    assert(query.empty());

    const std::string invalid{"\xE6\xB5", 2};
    assert(!lofibox::app::isValidUtf8(invalid));
    assert(lofibox::app::truncateUtf8(tokyo_jazz, 7) == "\xE6\x9D\xB1\xE4\xBA\xACJ");
    return 0;
}
