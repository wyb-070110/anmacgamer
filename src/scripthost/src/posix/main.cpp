// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// gs-scripthost off Windows: sandbox the process before anything is read (macOS: deny everything, as the
// pure-computation profile does), then serve the pipes the parent passed.
#include "Server.h"

#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#if defined(__APPLE__)
#include <cstdint>

// From <sandbox.h>, which this module's own Sandbox.h hides on a case-insensitive file system.
extern "C" {
int sandbox_init(const char* profile, std::uint64_t flags, char** error);
void sandbox_free_error(char* error);
}
#endif

namespace {

constexpr int kSandboxFailed = 4;

bool sandboxed() noexcept
{
#if defined(__APPLE__)
    // The profile is written out rather than named: macOS 27 kills a process that asks sandbox_init for any
    // named profile (kSBXProfilePureComputation included). Denying by default leaves the pipes already open
    // usable and refuses files, the network, other programs and IPC, which is what pure computation means.
    constexpr const char* kPureComputation = "(version 1)(deny default)";
    char* error = nullptr;
    const bool applied = sandbox_init(kPureComputation, 0, &error) == 0;
    if (error != nullptr) {
        sandbox_free_error(error);
    }
    return applied;
#else
    const char* allowed = std::getenv("GS_SCRIPTHOST_UNSANDBOXED");
    return allowed != nullptr && std::string_view(allowed) == "1";
#endif
}

}

int main(int argc, char** argv)
{
    std::vector<std::wstring> words;
    for (int index = 0; index < argc; ++index) {
        const std::string_view word(argv[index]);
        words.emplace_back(word.begin(), word.end());
        if (word == "--sandbox" && !sandboxed()) {
            return kSandboxFailed;
        }
    }
    std::vector<wchar_t*> pointers;
    for (std::wstring& word : words) {
        pointers.push_back(word.data());
    }
    pointers.push_back(nullptr);
    return gs::scripthost::serve(argc, pointers.data());
}
