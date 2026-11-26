// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// POSIX thread helpers: names, scheduling class, heap trimming and process age.
#include "gs/platform/Thread.h"

#include <pthread.h>

#include <chrono>
#include <cstddef>
#include <string>

#if defined(__APPLE__)
#include <libproc.h>
#include <malloc/malloc.h>
#include <pthread/qos.h>
#include <sys/proc_info.h>
#include <unistd.h>
#else
#include <fstream>
#include <malloc.h>
#include <sstream>
#include <time.h>
#include <unistd.h>
#endif

namespace gs::platform {

void setCurrentThreadName(std::string_view name) noexcept
{
    try {
#if defined(__APPLE__)
        const std::string text(name.substr(0, 63));
        pthread_setname_np(text.c_str());
#else
        const std::string text(name.substr(0, 15));
        pthread_setname_np(pthread_self(), text.c_str());
#endif
    } catch (...) {
    }
}

void setCurrentThreadRole(ThreadRole role) noexcept
{
#if defined(__APPLE__)
    pthread_set_qos_class_self_np(role == ThreadRole::Input ? QOS_CLASS_USER_INTERACTIVE : QOS_CLASS_USER_INITIATED, 0);
#else
    static_cast<void>(role);
#endif
}

bool keepTimerResolution() noexcept
{
    return true;
}

void returnFreeHeapMemory() noexcept
{
#if defined(__APPLE__)
    malloc_zone_pressure_relief(nullptr, 0);
#elif defined(__GLIBC__)
    malloc_trim(0);
#endif
}

std::int64_t millisecondsSinceProcessStart() noexcept
{
#if defined(__APPLE__)
    proc_bsdinfo info{};
    if (proc_pidinfo(getpid(), PROC_PIDTBSDINFO, 0, &info, PROC_PIDTBSDINFO_SIZE) != PROC_PIDTBSDINFO_SIZE) {
        return -1;
    }
    const auto started = std::chrono::system_clock::time_point(std::chrono::seconds(info.pbi_start_tvsec) +
                                                              std::chrono::microseconds(info.pbi_start_tvusec));
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - started).count();
#else
    try {
        std::ifstream stat("/proc/self/stat");
        std::string line;
        if (!std::getline(stat, line)) {
            return -1;
        }
        const std::size_t close = line.rfind(')');
        if (close == std::string::npos) {
            return -1;
        }
        std::istringstream fields(line.substr(close + 2));
        std::string field;
        for (int index = 3; index < 22 && fields >> field; ++index) {
        }
        unsigned long long startTicks = 0;
        if (!(fields >> startTicks)) {
            return -1;
        }
        const long ticksPerSecond = sysconf(_SC_CLK_TCK);
        timespec now{};
        if (ticksPerSecond <= 0 || clock_gettime(CLOCK_BOOTTIME, &now) != 0) {
            return -1;
        }
        const std::int64_t nowMs = static_cast<std::int64_t>(now.tv_sec) * 1000 + now.tv_nsec / 1'000'000;
        return nowMs - static_cast<std::int64_t>(startTicks * 1000 / static_cast<unsigned long long>(ticksPerSecond));
    } catch (...) {
        return -1;
    }
#endif
}

}
