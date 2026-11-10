// SPDX-License-Identifier: GPL-3.0-or-later
#include <iostream>
#include <stdexcept>
#include <string>
#include "runtime/runtime_envelope_serializer.h"

int main()
{
    using namespace lofibox::runtime;
    try {
        for (double value:{1e-9,1e-5,123.456,1e6,1e20,1e308,-1e-8}) {
            const auto encoded=serializeRuntimeCommandRequest({RuntimeCommand{RuntimeCommandKind::PlaybackSeek,RuntimeCommandPayload::seek(value),CommandOrigin::DirectTest}});
            const auto decoded=parseRuntimeCommandRequest(encoded);
            if (!decoded || !decoded->command.payload.get<PlaybackSeekPayload>()
                || decoded->command.payload.get<PlaybackSeekPayload>()->seconds!=value)
                throw std::runtime_error("numeric seek changed across the runtime wire: "+encoded);
        }
        for (const auto token:{"1e","1e+","1e999","1.2.3","12oops","NaN","\"12\"","01","+1"}) {
            const std::string wire="{\"kind\":\"PlaybackSeek\",\"payload\":\"PlaybackSeek\",\"seconds\":"+std::string(token)+"}";
            if (parseRuntimeCommandRequest(wire)) throw std::runtime_error("malformed seek number was accepted: "+wire);
        }
        if (parseRuntimeCommandRequest("{\"kind\":\"PlaybackSeek\",\"payload\":\"PlaybackSeek\"}"))
            throw std::runtime_error("missing seek value became zero");
        RuntimeSnapshot snapshot{};
        snapshot.playback.elapsed_seconds=1e-9;
        const auto response=serializeRuntimeQueryResponse({snapshot});
        const auto decoded=parseRuntimeQueryResponse(response);
        if (!decoded || decoded->snapshot.playback.elapsed_seconds!=1e-9)
            throw std::runtime_error("small snapshot time changed across the runtime wire");
        std::cout<<"Scientific notation survives command/query serialization; malformed seek numbers are rejected\n";
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
