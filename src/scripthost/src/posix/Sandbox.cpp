// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Starting gs-scripthost off Windows: two pipes, posix_spawn with only those inherited, and the child asked to
// sandbox itself (macOS: the pure-computation profile, no files, network or other programs) before it reads
// anything. A process group lets the parent end the child and anything it might start.
#include "Sandbox.h"

#include <fcntl.h>
#include <spawn.h>

#include <array>
#include <cstdlib>
#include <string_view>
#include <format>
#include <string>
#include <vector>

extern char** environ;

namespace gs::scripthost::detail {

namespace {

std::string failure(std::string_view what, int error)
{
    return std::format("{} ({})", what, std::strerror(error));
}

void closeQuietly(std::initializer_list<int> descriptors) noexcept
{
    for (const int fd : descriptors) {
        if (fd >= 0) {
            ::close(fd);
        }
    }
}

bool pipeOf(std::array<int, 2>& ends) noexcept
{
    std::array<int, 2> made{-1, -1};
    if (::pipe(made.data()) != 0) {
        return false;
    }
    for (std::size_t i = 0; i < made.size(); ++i) {
        ends[i] = ::fcntl(made[i], F_DUPFD_CLOEXEC, 10);
        ::close(made[i]);
    }
    return ends[0] >= 0 && ends[1] >= 0;
}

bool sandboxAvailable() noexcept
{
#if defined(__APPLE__)
    return true;
#else
    const char* allowed = std::getenv("GS_SCRIPTHOST_UNSANDBOXED");
    return allowed != nullptr && std::string_view(allowed) == "1";
#endif
}

}

std::optional<Launched> launch(const std::filesystem::path& program, SandboxMode mode, const std::wstring& extra,
                               std::string& problem)
{
    if (mode == SandboxMode::None || !sandboxAvailable()) {
        problem = "this system has no sandbox for gs-scripthost";
        return std::nullopt;
    }
    std::array<int, 2> down{-1, -1};
    std::array<int, 2> up{-1, -1};
    if (!pipeOf(down) || !pipeOf(up)) {
        problem = failure("the pipes to gs-scripthost could not be made", errno);
        closeQuietly({down[0], down[1], up[0], up[1]});
        return std::nullopt;
    }
    constexpr int kChildIn = 3;
    constexpr int kChildOut = 4;
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, down[0], kChildIn);
    posix_spawn_file_actions_adddup2(&actions, up[1], kChildOut);
    posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    posix_spawnattr_t attributes;
    posix_spawnattr_init(&attributes);
    short flags = POSIX_SPAWN_SETPGROUP;
#if defined(POSIX_SPAWN_CLOEXEC_DEFAULT)
    flags |= POSIX_SPAWN_CLOEXEC_DEFAULT;
    posix_spawn_file_actions_addinherit_np(&actions, STDOUT_FILENO);
    posix_spawn_file_actions_addinherit_np(&actions, STDERR_FILENO);
#endif
    posix_spawnattr_setflags(&attributes, flags);
    posix_spawnattr_setpgroup(&attributes, 0);

    const std::string path = program.string();
    std::vector<std::string> words{path, "--channel",
                                   std::to_string(kChildIn + 1) + "," + std::to_string(kChildOut + 1)};
    words.insert(words.end(), {"--sandbox", "pure"});
    std::string word;
    for (const wchar_t c : extra) {
        if (c == L' ') {
            if (!word.empty()) {
                words.push_back(std::exchange(word, {}));
            }
        } else {
            word.push_back(static_cast<char>(c));
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    std::vector<char*> arguments;
    for (std::string& each : words) {
        arguments.push_back(each.data());
    }
    arguments.push_back(nullptr);

    pid_t child = 0;
    const int spawned = posix_spawn(&child, path.c_str(), &actions, &attributes, arguments.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&attributes);
    closeQuietly({down[0], up[1]});
    if (spawned != 0) {
        problem = failure("gs-scripthost could not be started", spawned);
        closeQuietly({down[1], up[0]});
        return std::nullopt;
    }
    auto* process = new posix::ScriptProcess{.pid = child};
    return Launched{.process = process,
                    .job = nullptr,
                    .toChild = posix::descriptorHandle(down[1]),
                    .fromChild = posix::descriptorHandle(up[0]),
                    .id = static_cast<DWORD>(child),
                    .mode = mode};
}

void closeLaunched(Launched& launched) noexcept
{
    for (HANDLE handle : {launched.process, launched.job, launched.toChild, launched.fromChild}) {
        if (handle != nullptr) {
            CloseHandle(handle);
        }
    }
    launched = Launched{};
}

}
