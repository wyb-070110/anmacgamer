// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// A macOS dylib for the Mach-O checks: it exports gs_ext_main and, with GS_PROBE_EVERYTHING, imports functions
// the install check sorts into network, files, processes and injection. Nothing here is ever called.
#include <cstdio>

#if defined(GS_PROBE_EVERYTHING)
#include <mach/mach.h>
#include <spawn.h>
#include <sys/socket.h>
#endif

extern "C" {

__attribute__((visibility("default"))) int gs_ext_main(const void*, void*)
{
    return 0;
}

__attribute__((visibility("default"))) extern void* const gs_probe_uses[];
void* const gs_probe_uses[] = {
    reinterpret_cast<void*>(&std::fopen),
#if defined(GS_PROBE_EVERYTHING)
    reinterpret_cast<void*>(&::socket),
    reinterpret_cast<void*>(&::connect),
    reinterpret_cast<void*>(&::posix_spawn),
    reinterpret_cast<void*>(&::task_for_pid),
#endif
};

}
