// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Starting gs-exthost off Windows (posix/Bridge.h): it inherits only what it is given, as Windows hands it only the
// channel's handles (macOS: POSIX_SPAWN_CLOEXEC_DEFAULT with an inherit list; Linux: fork and exec).
#include "Bridge.h"

#include <signal.h>
#include <spawn.h>
#include <sys/socket.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <crt_externs.h>
#endif

#include <array>
#include <cerrno>
#include <cstring>
#include <string_view>

namespace gs::exthost::detail::posix {

std::optional<Spawned> spawnHost(const std::filesystem::path& program, const std::vector<std::string>& arguments,
                                 std::vector<std::string> environment, const std::filesystem::path& folder,
                                 std::vector<int> inherited, std::string& problem)
{
    std::array<int, 2> bridge{-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, bridge.data()) != 0) {
        problem = "the bridge to gs-exthost could not be made (" + std::string(std::strerror(errno)) + ")";
        return std::nullopt;
    }
    closeOnExec(bridge[0], true);
    closeOnExec(bridge[1], true);
#if defined(SO_NOSIGPIPE)
    for (const int end : bridge) {
        const int on = 1;
        ::setsockopt(end, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
    }
#endif
    inherited.insert(inherited.end(), {0, 1, 2, bridge[1]});
    environment.push_back(std::string(kBridgeVariable) + "=" + std::to_string(bridge[1]));

    std::vector<std::string> words{program.string()};
    words.insert(words.end(), arguments.begin(), arguments.end());
    std::vector<char*> argv;
    for (std::string& word : words) {
        argv.push_back(word.data());
    }
    argv.push_back(nullptr);
    std::vector<char*> envp;
    for (std::string& entry : environment) {
        envp.push_back(entry.data());
    }
    envp.push_back(nullptr);

    pid_t pid = 0;
    int error = 0;
#if defined(__APPLE__)
    posix_spawn_file_actions_t actions;
    posix_spawnattr_t attributes;
    posix_spawn_file_actions_init(&actions);
    posix_spawnattr_init(&attributes);
    posix_spawnattr_setflags(&attributes, POSIX_SPAWN_CLOEXEC_DEFAULT | POSIX_SPAWN_SETSIGDEF);
    sigset_t defaults;
    sigfillset(&defaults);
    posix_spawnattr_setsigdefault(&attributes, &defaults);
    for (const int fd : inherited) {
        posix_spawn_file_actions_addinherit_np(&actions, fd);
    }
    if (!folder.empty()) {
        // macOS 26 renamed it posix_spawn_file_actions_addchdir; macOS 15 has only this name.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        posix_spawn_file_actions_addchdir_np(&actions, folder.c_str());
#pragma clang diagnostic pop
    }
    error = ::posix_spawn(&pid, words.front().c_str(), &actions, &attributes, argv.data(), envp.data());
    posix_spawn_file_actions_destroy(&actions);
    posix_spawnattr_destroy(&attributes);
#else
    pid = ::fork();
    if (pid == 0) {
        for (const int fd : inherited) {
            closeOnExec(fd, false);
        }
        if (!folder.empty() && ::chdir(folder.c_str()) != 0) {
            ::_exit(127);
        }
        ::execve(words.front().c_str(), argv.data(), envp.data());
        ::_exit(127);
    }
    error = pid < 0 ? errno : 0;
#endif
    ::close(bridge[1]);
    if (error != 0) {
        ::close(bridge[0]);
        problem = "gs-exthost could not be started (" + std::string(std::strerror(error)) + ")";
        return std::nullopt;
    }
    return Spawned{pid, bridge[0]};
}

std::vector<std::string> currentEnvironment()
{
#if defined(__APPLE__)
    char** entries = *_NSGetEnviron();
#else
    char** entries = environ;
#endif
    std::vector<std::string> out;
    for (char** entry = entries; entry != nullptr && *entry != nullptr; ++entry) {
        if (!std::string_view(*entry).starts_with(kBridgeVariable)) {
            out.emplace_back(*entry);
        }
    }
    return out;
}

}
