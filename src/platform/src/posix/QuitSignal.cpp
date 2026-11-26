// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Self-pipe watcher for SIGTERM, SIGINT and SIGHUP: the first signal wakes the event loop to quit cleanly; a
// second signal while quitting restores the default disposition and exits at once.
#include "gs/platform/QuitSignal.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <atomic>
#include <cstddef>

namespace gs::platform {

namespace {

constexpr std::array<int, 3> kSignals{SIGTERM, SIGINT, SIGHUP};

std::atomic<bool> g_installed{false};
std::atomic<int> g_writeFd{-1};
std::atomic<int> g_received{0};
std::array<struct sigaction, kSignals.size()> g_before{};

bool configureDescriptor(int fd) noexcept
{
    const int flags = ::fcntl(fd, F_GETFL, 0);
    return flags >= 0 && ::fcntl(fd, F_SETFD, FD_CLOEXEC) == 0 && ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

void onSignal(int signal) noexcept
{
    const int savedErrno = errno;
    const int count = g_received.fetch_add(1, std::memory_order_relaxed) + 1;
    if (count == 1) {
        const int fd = g_writeFd.load(std::memory_order_relaxed);
        if (fd >= 0) {
            const auto byte = static_cast<unsigned char>(signal);
            const ssize_t wrote = ::write(fd, &byte, 1);
            static_cast<void>(wrote);
        }
        errno = savedErrno;
        return;
    }
    struct sigaction usual{};
    usual.sa_handler = SIG_DFL;
    sigemptyset(&usual.sa_mask);
    ::sigaction(signal, &usual, nullptr);
    sigset_t mask{};
    sigemptyset(&mask);
    sigaddset(&mask, signal);
    ::pthread_sigmask(SIG_UNBLOCK, &mask, nullptr);
    ::raise(signal);
    ::_exit(128 + signal);
}

}

QuitSignal::QuitSignal()
{
    bool expected = false;
    if (!g_installed.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return;
    }
    if (::pipe(m_pipe.data()) != 0) {
        g_installed.store(false, std::memory_order_release);
        return;
    }
    if (!configureDescriptor(m_pipe[0]) || !configureDescriptor(m_pipe[1])) {
        ::close(m_pipe[0]);
        ::close(m_pipe[1]);
        m_pipe = {-1, -1};
        g_installed.store(false, std::memory_order_release);
        return;
    }
    g_received.store(0, std::memory_order_relaxed);
    g_writeFd.store(m_pipe[1], std::memory_order_release);

    sigset_t unblock{};
    sigemptyset(&unblock);
    struct sigaction action{};
    action.sa_handler = &onSignal;
    action.sa_flags = SA_RESTART;
    sigemptyset(&action.sa_mask);
    for (std::size_t i = 0; i < kSignals.size(); ++i) {
        sigaddset(&unblock, kSignals[i]);
        ::sigaction(kSignals[i], &action, &g_before[i]);
    }
    ::pthread_sigmask(SIG_UNBLOCK, &unblock, nullptr);
}

QuitSignal::~QuitSignal()
{
    if (m_pipe[0] < 0) {
        return;
    }
    for (std::size_t i = 0; i < kSignals.size(); ++i) {
        ::sigaction(kSignals[i], &g_before[i], nullptr);
    }
    g_writeFd.store(-1, std::memory_order_release);
    g_received.store(0, std::memory_order_relaxed);
    ::close(m_pipe[1]);
    ::close(m_pipe[0]);
    m_pipe = {-1, -1};
    g_installed.store(false, std::memory_order_release);
}

std::optional<int> QuitSignal::take() noexcept
{
    if (m_pipe[0] < 0) {
        return std::nullopt;
    }
    unsigned char byte = 0;
    if (::read(m_pipe[0], &byte, 1) != 1) {
        return std::nullopt;
    }
    return static_cast<int>(byte);
}

std::string_view quitSignalName(int signal) noexcept
{
    switch (signal) {
    case SIGTERM:
        return "SIGTERM";
    case SIGINT:
        return "SIGINT";
    case SIGHUP:
        return "SIGHUP";
    default:
        return "signal";
    }
}

}
