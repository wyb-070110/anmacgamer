// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Catches termination signals (SIGTERM, SIGINT, SIGHUP) on POSIX systems through a self-pipe so the main event
// loop can quit cleanly; a second signal while quitting terminates at once.
#pragma once

#include <array>
#include <optional>
#include <string_view>

namespace gs::platform {

class QuitSignal
{
public:
    QuitSignal();
    ~QuitSignal();
    QuitSignal(const QuitSignal&) = delete;
    QuitSignal& operator=(const QuitSignal&) = delete;

    [[nodiscard]] int descriptor() const noexcept { return m_pipe[0]; }
    [[nodiscard]] std::optional<int> take() noexcept;

private:
    std::array<int, 2> m_pipe{-1, -1};
};

[[nodiscard]] std::string_view quitSignalName(int signal) noexcept;

}
