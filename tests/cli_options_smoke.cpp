// SPDX-License-Identifier: GPL-3.0-or-later

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

#include "targets/cli_options.h"

namespace {

bool run(std::vector<std::string> args, std::string& out_text)
{
    std::vector<char*> argv{};
    argv.reserve(args.size());
    for (auto& arg : args) {
        argv.push_back(arg.data());
    }
    std::ostringstream out{};
    const bool handled = lofibox::targets::handleCommonCliOptions(static_cast<int>(argv.size()), argv.data(), out);
    out_text = out.str();
    return handled;
}

} // namespace

int main()
{
    std::string out{};
    bool handled = run({"lofibox", "version", "--json"}, out);
    assert(handled);
    assert(out.find("\"name\":\"lofibox\"") != std::string::npos);

    handled = run({"lofibox", "--json", "version"}, out);
    assert(handled);
    assert(out.find("\"version\"") != std::string::npos);

    handled = run({"lofibox", "--version", "--porcelain"}, out);
    assert(handled);
    assert(out.find("version\t") != std::string::npos);

    handled = run({"lofibox", "--quiet", "version"}, out);
    assert(handled);
    assert(out.empty());

    handled = run({"lofibox", "source", "list"}, out);
    assert(!handled);

    return 0;
}
