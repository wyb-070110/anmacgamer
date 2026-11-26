// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// gs-exthost off Windows, included by Hosted.cpp in its unnamed namespace in place of the Windows launch.
//
// gs-exthost inherits only the channel's descriptors, its end of the bridge and the standard ones, as Windows hands
// it only the channel's handles (macOS: POSIX_SPAWN_CLOEXEC_DEFAULT with an inherit list; Linux: fork and exec).
// Restricted: no --parent, a short environment, the container folder as its home and working folder, and
// gs-exthost enters its sandbox before it reads the channel (posix/Sandbox.h). macOS has no job object: the memory
// and process limits of Windows are not applied, but the sandbox already refuses starting other programs.

std::string narrowText(const std::wstring& text)
{
    std::string out;
    for (const wchar_t c : text) {
        out.push_back(static_cast<char>(c));
    }
    return out;
}

std::vector<int> channelDescriptors(const detail::Channel& channel)
{
    std::vector<int> fds;
    for (const HANDLE handle : channel.handles()) {
        for (const int fd : detail::posix::descriptorsOf(handle)) {
            fds.push_back(fd);
        }
    }
    return fds;
}

std::vector<std::string> channelArguments(const detail::Channel& channel)
{
    return {"--channel", narrowText(detail::handleText(channel.handles()))};
}

std::optional<Instance> launchPlain(const HostedOptions& options, detail::Channel channel, std::string& problem)
{
    std::vector<std::string> arguments = channelArguments(channel);
    arguments.insert(arguments.end(), {"--parent", std::to_string(GetCurrentProcessId())});
    if (!options.crashes.empty()) {
        arguments.insert(arguments.end(), {"--crashes", options.crashes.string(), "--log", options.logFile.string()});
    }
    const std::optional<detail::posix::Spawned> spawned =
        detail::posix::spawnHost(options.program, arguments, detail::posix::currentEnvironment(),
                                 options.program.parent_path(), channelDescriptors(channel), problem);
    if (!spawned) {
        return std::nullopt;
    }
    const HANDLE process = detail::posix::adoptChild(spawned->pid, spawned->bridge);
    if (process == nullptr) {
        problem = "gs-exthost could not be watched";
        return std::nullopt;
    }
    return Instance{std::move(channel), process, nullptr};
}

std::optional<Instance> launchContained(const HostedOptions& options, detail::Channel channel, std::string& problem)
{
#if defined(__APPLE__)
    const std::filesystem::path container = detail::posix::containerFolder(options.container, problem);
    if (container.empty()) {
        return std::nullopt;
    }
    std::error_code error;
    const std::filesystem::path readable = std::filesystem::canonical(
        options.readable.empty() ? options.dll.parent_path() : options.readable, error);
    if (error) {
        problem = "the extension's folder cannot be found: " + error.message();
        return std::nullopt;
    }
    const std::filesystem::path program = std::filesystem::canonical(options.program, error);
    if (error) {
        problem = "gs-exthost cannot be found: " + error.message();
        return std::nullopt;
    }
    std::vector<std::string> arguments = channelArguments(channel);
    arguments.insert(arguments.end(), {"--sandbox", "restricted"});
    std::vector<std::string> environment{
        "HOME=" + container.string(),
        "TMPDIR=" + (container / "tmp").string() + "/",
        "PATH=/usr/bin:/bin",
        std::string(detail::posix::kReadableVariable) + "=" + readable.string(),
        std::string(detail::posix::kContainerVariable) + "=" + container.string(),
        std::string(detail::posix::kProgramVariable) + "=" + program.string(),
    };
    for (const char* name : {"LANG", "LC_ALL"}) {
        if (const char* value = std::getenv(name)) {
            environment.push_back(std::string(name) + "=" + value);
        }
    }
    const std::optional<detail::posix::Spawned> spawned = detail::posix::spawnHost(
        program, arguments, std::move(environment), container, channelDescriptors(channel), problem);
    if (!spawned) {
        return std::nullopt;
    }
    const HANDLE process = detail::posix::adoptChild(spawned->pid, spawned->bridge);
    if (process == nullptr) {
        problem = "gs-exthost could not be watched";
        return std::nullopt;
    }
    const std::string name = narrowText(detail::containerName(options.container));
    detail::posix::joinContainer(process, name);
    for (const std::string& token : detail::posix::sharedTokens(name)) {
        detail::posix::grantToProcess(process, token);
    }
    return Instance{std::move(channel), process, nullptr};
#else
    static_cast<void>(options);
    static_cast<void>(channel);
    problem = "restricted extensions need the macOS sandbox";
    return std::nullopt;
#endif
}
