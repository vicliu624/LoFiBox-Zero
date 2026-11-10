// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_host_internal.h"
#include "core/operation_cancellation.h"

#include <cassert>
#include <filesystem>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#if defined(__linux__)
#include <sys/wait.h>
#endif

int main()
{
    using lofibox::platform::host::runtime_detail::parseJsonStringAt;
    assert(parseJsonStringAt(R"("\u4e2d\u6587")", 0) == "\xE4\xB8\xAD\xE6\x96\x87");
    assert(parseJsonStringAt(R"("\ud83c\udfb5")", 0) == "\xF0\x9F\x8E\xB5");
    assert(parseJsonStringAt(R"("\udbff\udfff")", 0) == "\xF4\x8F\xBF\xBF");
    assert(parseJsonStringAt(R"("quote\" slash\\ line\n")", 0) == "quote\" slash\\ line\n");
    for (const auto invalid : {R"("\ud800")", R"("\ud800x")", R"("\ud800\u0041")", R"("\udfff")",
                               R"("\uZZZZ")", R"("\u123)", R"("unterminated)"}) {
        assert(!parseJsonStringAt(invalid, 0));
    }
#if defined(__linux__)
    using lofibox::platform::host::runtime_detail::captureProcessOutput;
    using lofibox::platform::host::runtime_detail::runProcess;

    const std::filesystem::path shell{"/bin/sh"};
    const auto output = captureProcessOutput(shell, {"-c", "printf 'metadata output'"});
    assert(output);
    assert(*output == "metadata output");

    assert(runProcess(shell, {"-c", "exit 0"}));
    assert(!runProcess(shell, {"-c", "exit 7"}));
    std::stop_source cancellation;
    std::thread canceller([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        cancellation.request_stop();
    });
    const auto started = std::chrono::steady_clock::now();
    {
        const lofibox::core::OperationCancellationScope scope{cancellation.get_token()};
        const auto cancelled = captureProcessOutput(shell, {"-c", "sleep 60"});
        assert(!cancelled);
    }
    canceller.join();
    assert(std::chrono::steady_clock::now() - started < std::chrono::seconds{1});
    // A cancelled operation must not leak its cancellation into later work.
    assert(captureProcessOutput(shell, {"-c", "printf 'after cancellation'"}) == "after cancellation");
    int status{};
    assert(::waitpid(-1, &status, WNOHANG) == -1);
#endif
    return 0;
}
