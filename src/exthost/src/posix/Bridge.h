// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// The handles of posix/compat/windows.h as the POSIX launch of gs-exthost needs them, and the bridge: a Unix socket
// between the app and each gs-exthost over which DuplicateHandle passes descriptors (SCM_RIGHTS).
//
// App -> child: the app sends the descriptors with a number; the child's handle is that number (tag 4) until it
//               is used, when it becomes the descriptors that arrived with it.
// Child -> app: the app asks for a handle value the child named; the child's bridge thread answers with the
//               descriptors behind it.
// The child finds its end in GS_EXTHOST_BRIDGE. It reads the bridge on a thread of its own, started on its first
// use of a handle, and leaves at once when the app's end closes: the app is gone.
#pragma once

#include <windows.h>

#include <sys/types.h>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gs::exthost::detail::posix {

inline constexpr const char* kBridgeVariable = "GS_EXTHOST_BRIDGE";

// The descriptors behind an event, file mapping, watch or file handle (what a child must inherit to use it).
[[nodiscard]] std::vector<int> descriptorsOf(HANDLE handle);

// A process handle for a child just started, with the app's end of its bridge (the handle owns it).
[[nodiscard]] HANDLE adoptChild(pid_t pid, int bridge) noexcept;

// The process id behind a process handle, or 0.
[[nodiscard]] pid_t processIdOf(HANDLE process) noexcept;

// Sets the close-on-exec flag of a descriptor.
bool closeOnExec(int descriptor, bool on) noexcept;

// Starts gs-exthost with only the given descriptors (and the standard ones) inherited, plus its end of a new bridge
// named in the environment; the app's end and the process id come back.
struct Spawned {
    pid_t pid = 0;
    int bridge = -1;
};
[[nodiscard]] std::optional<Spawned> spawnHost(const std::filesystem::path& program,
                                               const std::vector<std::string>& arguments,
                                               std::vector<std::string> environment,
                                               const std::filesystem::path& folder, std::vector<int> inherited,
                                               std::string& problem);

// This process's environment, without the bridge variable.
[[nodiscard]] std::vector<std::string> currentEnvironment();

// Restricted processes by container: a sandbox extension token (macOS) given to a container reaches every one of
// its processes now running; the child consumes it, which lets it read the one file the token names.
void joinContainer(HANDLE process, const std::string& container);
void grantToContainer(const std::string& container, std::string_view token);
bool grantToProcess(HANDLE process, std::string_view token);

}
