// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Restricted native extensions off Windows: what stands in for the AppContainer of Container.cpp.
//
// macOS: gs-exthost puts itself in a sandbox (sandbox_init with a profile written out here) before it reads the
// channel. The sandbox lets it read the extension's folder and the system's libraries, read and write the
// extension's own container folder, and use the descriptors it was given; files, the network, other programs and
// other services are refused. A file the app shares later (shareWithContainer) comes as a sandbox extension token.
// Linux has no such sandbox here: a restricted extension does not start there.
#pragma once

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gs::exthost::detail::posix {

inline constexpr const char* kReadableVariable = "GS_EXTHOST_READABLE";
inline constexpr const char* kContainerVariable = "GS_EXTHOST_CONTAINER";
inline constexpr const char* kProgramVariable = "GS_EXTHOST_PROGRAM";

// ~/Library/Application Support/anmacgamer/Containers/GameScrcpy.Extension.<hash> (Linux: under XDG_DATA_HOME),
// made with mode 0700 and a tmp folder inside; empty if it cannot be made.
[[nodiscard]] std::filesystem::path containerFolder(std::string_view extension, std::string& problem);

// The sandbox profile for a restricted gs-exthost; the paths must be real (no symbolic links).
[[nodiscard]] std::string sandboxProfile(const std::filesystem::path& readable, const std::filesystem::path& container,
                                         const std::filesystem::path& program);

// Puts this process in the sandbox described by the variables above; false (and why) if it could not.
[[nodiscard]] bool enterSandbox(std::string& problem);

// The tokens shared with a container so far, given again to each new process of it.
[[nodiscard]] std::vector<std::string> sharedTokens(const std::string& container);

}
