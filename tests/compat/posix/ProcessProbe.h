// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Test builds off Windows: what the adb and session tests read about processes (named threads, open
// descriptors, private memory, children, the owner of a listening port), from /proc on Linux and the Mach
// and libproc interfaces on macOS.
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#if defined(__APPLE__)
#include <arpa/inet.h>
#include <libproc.h>
#include <mach/mach.h>
#include <pthread.h>
#include <sys/proc_info.h>
#include <unistd.h>
#else
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unistd.h>
#endif

#include <vector>

namespace gs::test {

inline int threadsNamed(std::string_view prefix)
{
    int count = 0;
#if defined(__APPLE__)
    thread_act_array_t threads = nullptr;
    mach_msg_type_number_t threadCount = 0;
    if (task_threads(mach_task_self(), &threads, &threadCount) != KERN_SUCCESS) {
        return 0;
    }
    for (mach_msg_type_number_t i = 0; i < threadCount; ++i) {
        if (const pthread_t thread = pthread_from_mach_thread_np(threads[i]); thread != nullptr) {
            std::array<char, 64> name{};
            if (pthread_getname_np(thread, name.data(), name.size()) == 0 &&
                std::string_view(name.data()).starts_with(prefix)) {
                ++count;
            }
        }
        mach_port_deallocate(mach_task_self(), threads[i]);
    }
    vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(threads), threadCount * sizeof(thread_act_t));
#else
    std::error_code error;
    for (const auto& task : std::filesystem::directory_iterator("/proc/self/task", error)) {
        std::ifstream comm(task.path() / "comm");
        std::string name;
        if (std::getline(comm, name) && name.starts_with(prefix)) {
            ++count;
        }
    }
#endif
    return count;
}

inline int allThreads()
{
    return threadsNamed("");
}

inline std::uint32_t openHandles()
{
#if defined(__APPLE__)
    const int bytes = proc_pidinfo(getpid(), PROC_PIDLISTFDS, 0, nullptr, 0);
    return bytes > 0 ? static_cast<std::uint32_t>(static_cast<std::size_t>(bytes) / sizeof(proc_fdinfo)) : 0;
#else
    std::uint32_t count = 0;
    std::error_code error;
    for (auto it = std::filesystem::directory_iterator("/proc/self/fd", error);
         !error && it != std::filesystem::directory_iterator(); it.increment(error)) {
        ++count;
    }
    return count;
#endif
}

inline double privateMegabytes()
{
#if defined(__APPLE__)
    task_vm_info_data_t info{};
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&info), &count) != KERN_SUCCESS) {
        return 0.0;
    }
    return static_cast<double>(info.phys_footprint) / (1024.0 * 1024.0);
#else
    std::ifstream status("/proc/self/status");
    for (std::string line; std::getline(status, line);) {
        if (line.starts_with("RssAnon:")) {
            return static_cast<double>(std::stoull(line.substr(8))) / 1024.0;
        }
    }
    return 0.0;
#endif
}

#if defined(__APPLE__)
inline std::vector<pid_t> allProcesses()
{
    std::vector<pid_t> pids(static_cast<std::size_t>(std::max(proc_listallpids(nullptr, 0), 0)) + 64);
    const int count = proc_listallpids(pids.data(), static_cast<int>(pids.size() * sizeof(pid_t)));
    pids.resize(count > 0 ? static_cast<std::size_t>(count) : 0);
    return pids;
}
#endif

inline int childProcesses()
{
    int count = 0;
#if defined(__APPLE__)
    for (const pid_t pid : allProcesses()) {
        proc_bsdinfo info{};
        if (proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &info, PROC_PIDTBSDINFO_SIZE) == PROC_PIDTBSDINFO_SIZE &&
            static_cast<pid_t>(info.pbi_ppid) == getpid()) {
            ++count;
        }
    }
#else
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator("/proc", error)) {
        std::ifstream stat(entry.path() / "stat");
        std::string line;
        if (!std::getline(stat, line)) {
            continue;
        }
        const std::size_t close = line.rfind(')');
        std::istringstream fields(close == std::string::npos ? std::string{} : line.substr(close + 2));
        std::string state;
        long parent = 0;
        if (fields >> state >> parent && parent == static_cast<long>(getpid())) {
            ++count;
        }
    }
#endif
    return count;
}

inline std::uint32_t tcpListenerProcess(std::uint16_t port)
{
#if defined(__APPLE__)
    for (const pid_t pid : allProcesses()) {
        const int bytes = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, nullptr, 0);
        if (bytes <= 0) {
            continue;
        }
        std::vector<proc_fdinfo> fds(static_cast<std::size_t>(bytes) / sizeof(proc_fdinfo) + 16);
        const int filled = proc_pidinfo(pid, PROC_PIDLISTFDS, 0, fds.data(), static_cast<int>(fds.size() * sizeof(proc_fdinfo)));
        fds.resize(filled > 0 ? static_cast<std::size_t>(filled) / sizeof(proc_fdinfo) : 0);
        for (const proc_fdinfo& fd : fds) {
            if (fd.proc_fdtype != PROX_FDTYPE_SOCKET) {
                continue;
            }
            socket_fdinfo socket{};
            if (proc_pidfdinfo(pid, fd.proc_fd, PROC_PIDFDSOCKETINFO, &socket, PROC_PIDFDSOCKETINFO_SIZE) !=
                    PROC_PIDFDSOCKETINFO_SIZE ||
                socket.psi.soi_kind != SOCKINFO_TCP) {
                continue;
            }
            const tcp_sockinfo& tcp = socket.psi.soi_proto.pri_tcp;
            if (tcp.tcpsi_state == TSI_S_LISTEN && ntohs(static_cast<std::uint16_t>(tcp.tcpsi_ini.insi_lport)) == port) {
                return static_cast<std::uint32_t>(pid);
            }
        }
    }
    return 0;
#else
    std::string inode;
    for (const char* table : {"/proc/net/tcp", "/proc/net/tcp6"}) {
        std::ifstream lines(table);
        std::string line;
        std::getline(lines, line);
        while (inode.empty() && std::getline(lines, line)) {
            std::istringstream fields(line);
            std::string slot, local, remote, state, queues, timer, retransmits, uid, timeout, node;
            if (!(fields >> slot >> local >> remote >> state >> queues >> timer >> retransmits >> uid >> timeout >> node)) {
                continue;
            }
            const std::size_t colon = local.rfind(':');
            if (state == "0A" && colon != std::string::npos &&
                std::stoul(local.substr(colon + 1), nullptr, 16) == port) {
                inode = node;
            }
        }
    }
    if (inode.empty()) {
        return 0;
    }
    const std::string target = "socket:[" + inode + "]";
    std::error_code error;
    for (const auto& process : std::filesystem::directory_iterator("/proc", error)) {
        std::error_code inner;
        for (const auto& fd : std::filesystem::directory_iterator(process.path() / "fd", inner)) {
            std::error_code link;
            if (std::filesystem::read_symlink(fd.path(), link).string() == target) {
                return static_cast<std::uint32_t>(std::stoul(process.path().filename().string()));
            }
        }
    }
    return 0;
#endif
}

}
