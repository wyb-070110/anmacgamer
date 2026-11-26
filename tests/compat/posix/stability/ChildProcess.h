// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Test builds off Windows: the ChildProcess of tests/stability on posix_spawn, waitpid and kill.
#pragma once

#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <thread>
#include <vector>

extern char** environ;

namespace gs::test {

class ChildProcess
{
public:
    ChildProcess(const std::filesystem::path& program, const std::vector<std::wstring>& arguments)
    {
        std::vector<std::string> words{program.string()};
        for (const std::wstring& argument : arguments) {
            words.push_back(std::filesystem::path(argument).string());
        }
        std::vector<char*> pointers;
        for (std::string& word : words) {
            pointers.push_back(word.data());
        }
        pointers.push_back(nullptr);
        pid_t child = 0;
        if (::posix_spawn(&child, words.front().c_str(), nullptr, nullptr, pointers.data(), environ) == 0) {
            m_pid = child;
        }
    }

    ~ChildProcess() { kill(); }

    ChildProcess(const ChildProcess&) = delete;
    ChildProcess& operator=(const ChildProcess&) = delete;

    [[nodiscard]] bool started() const noexcept { return m_pid > 0; }

    [[nodiscard]] bool running() const noexcept { return started() && !reap(false); }

    [[nodiscard]] bool waitFor(std::chrono::milliseconds limit) const noexcept
    {
        const auto deadline = std::chrono::steady_clock::now() + limit;
        while (started() && !reap(false)) {
            if (std::chrono::steady_clock::now() >= deadline) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5)); // source-rule-allow R9: no waitpid with a timeout
        }
        return started();
    }

    void kill() noexcept
    {
        if (running()) {
            ::kill(m_pid, SIGKILL);
        }
        if (started()) {
            reap(true);
        }
    }

    [[nodiscard]] std::optional<std::uint32_t> exitCode() const noexcept
    {
        if (!started() || !reap(false)) {
            return std::nullopt;
        }
        if (WIFEXITED(m_status)) {
            return static_cast<std::uint32_t>(WEXITSTATUS(m_status));
        }
        return 1;
    }

private:
    bool reap(bool block) const noexcept
    {
        if (m_reaped) {
            return true;
        }
        for (;;) {
            const pid_t done = ::waitpid(m_pid, &m_status, block ? 0 : WNOHANG);
            if (done == m_pid || (done < 0 && errno != EINTR)) {
                m_reaped = true;
                return true;
            }
            if (done == 0) {
                return false;
            }
        }
    }

    pid_t m_pid = 0;
    mutable bool m_reaped = false;
    mutable int m_status = 0;
};

}
