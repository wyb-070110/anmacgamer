// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// gs-exthost off Windows: the arguments as wide strings, as wmain gets them on Windows. A restricted gs-exthost
// enters its sandbox here, before it reads the channel or loads the library (posix/Sandbox.h).
#include <gs/exthost/Server.h>

#include "../src/posix/Sandbox.h"

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr int kSandboxFailed = 6;

}

int main(int argc, char** argv)
{
    std::vector<std::wstring> words;
    bool restricted = false;
    for (int index = 0; index < argc; ++index) {
        const std::string_view word(argv[index]);
        words.emplace_back(word.begin(), word.end());
        if (word == "--sandbox" && index + 1 < argc && std::string_view(argv[index + 1]) == "restricted") {
            restricted = true;
        }
    }
    if (restricted) {
        std::string problem;
        if (!gs::exthost::detail::posix::enterSandbox(problem)) {
            std::fprintf(stderr, "gs-exthost: the sandbox could not be entered: %s\n", problem.c_str());
            return kSandboxFailed;
        }
    }
    std::vector<wchar_t*> pointers;
    for (std::wstring& word : words) {
        pointers.push_back(word.data());
    }
    pointers.push_back(nullptr);
    return gs::exthost::serve(argc, pointers.data());
}
