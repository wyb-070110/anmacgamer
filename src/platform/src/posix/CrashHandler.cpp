// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Crash reports off Windows: on a fatal signal, a folder crash-YYYYMMDD-HHMMSS-mmm under the given directory with
// crash.txt (version, time, signal, address and the module it is in, thread) and a copy of the log, as on Windows
// (which also writes a minidump; there is none here). Then the signal takes its usual course.
//
// The handler runs on its own stack and uses only calls that are safe in a signal handler, except dladdr to name
// the module, which every crash reporter on these systems uses.
#include "gs/platform/CrashHandler.h"

#include <gs/base/BuildInfo.h>

#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <sys/ucontext.h>
#else
#include <sys/syscall.h>
#include <ucontext.h>
#endif

#include <array>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string_view>

namespace gs::platform {

namespace {

constexpr std::size_t kMostPath = 4096;
constexpr std::array<int, 6> kSignals{SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT, SIGTRAP};

std::array<char, kMostPath> g_directory{};
std::array<char, kMostPath> g_log{};
std::atomic<bool> g_crashing{false};

class Text
{
public:
    void add(std::string_view text) noexcept
    {
        for (const char c : text) {
            if (m_size < m_data.size()) {
                m_data[m_size++] = c;
            }
        }
    }

    void add(char c) noexcept { add(std::string_view(&c, 1)); }

    void number(std::uint64_t value, unsigned base, int width) noexcept
    {
        std::array<char, 24> digits{};
        int count = 0;
        do {
            digits[static_cast<std::size_t>(count++)] = "0123456789abcdef"[value % base];
            value /= base;
        } while (value != 0 && count < static_cast<int>(digits.size()));
        for (int pad = count; pad < width; ++pad) {
            add('0');
        }
        while (count > 0) {
            add(digits[static_cast<std::size_t>(--count)]);
        }
    }

    [[nodiscard]] const char* c_str() noexcept
    {
        m_data[std::min(m_size, m_data.size() - 1)] = '\0';
        return m_data.data();
    }

    [[nodiscard]] std::string_view view() const noexcept { return {m_data.data(), m_size}; }

private:
    std::array<char, 4096> m_data{};
    std::size_t m_size = 0;
};

struct Civil {
    std::int64_t year = 0;
    unsigned month = 0;
    unsigned day = 0;
};

// Days since 1970-01-01 to a date (Howard Hinnant's algorithm): gmtime is not safe in a signal handler.
Civil civil(std::int64_t days) noexcept
{
    days += 719468;
    const std::int64_t era = (days >= 0 ? days : days - 146096) / 146097;
    const auto dayOfEra = static_cast<unsigned>(days - era * 146097);
    const unsigned yearOfEra = (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
    const unsigned dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
    const unsigned shifted = (5 * dayOfYear + 2) / 153;
    const unsigned day = dayOfYear - (153 * shifted + 2) / 5 + 1;
    const unsigned month = shifted < 10 ? shifted + 3 : shifted - 9;
    return {static_cast<std::int64_t>(yearOfEra) + era * 400 + (month <= 2 ? 1 : 0), month, day};
}

std::string_view signalName(int signal) noexcept
{
    switch (signal) {
    case SIGSEGV:
        return "SIGSEGV";
    case SIGBUS:
        return "SIGBUS";
    case SIGILL:
        return "SIGILL";
    case SIGFPE:
        return "SIGFPE";
    case SIGABRT:
        return "SIGABRT";
    case SIGTRAP:
        return "SIGTRAP";
    default:
        return {};
    }
}

std::uintptr_t programCounter(void* context) noexcept
{
    if (context == nullptr) {
        return 0;
    }
    const auto* user = static_cast<const ucontext_t*>(context);
#if defined(__APPLE__) && defined(__aarch64__)
    return static_cast<std::uintptr_t>(__darwin_arm_thread_state64_get_pc(user->uc_mcontext->__ss));
#elif defined(__APPLE__) && defined(__x86_64__)
    return static_cast<std::uintptr_t>(user->uc_mcontext->__ss.__rip);
#elif defined(__aarch64__)
    return static_cast<std::uintptr_t>(user->uc_mcontext.pc);
#elif defined(__x86_64__)
    return static_cast<std::uintptr_t>(user->uc_mcontext.gregs[REG_RIP]);
#else
    return 0;
#endif
}

std::uint64_t threadNumber() noexcept
{
#if defined(__APPLE__)
    std::uint64_t id = 0;
    pthread_threadid_np(nullptr, &id);
    return id;
#else
    return static_cast<std::uint64_t>(::syscall(SYS_gettid));
#endif
}

void writeAll(int fd, std::string_view bytes) noexcept
{
    while (!bytes.empty()) {
        const ssize_t wrote = ::write(fd, bytes.data(), bytes.size());
        if (wrote <= 0) {
            return;
        }
        bytes.remove_prefix(static_cast<std::size_t>(wrote));
    }
}

void copyLog(const char* folder) noexcept
{
    if (g_log[0] == '\0') {
        return;
    }
    const int in = ::open(g_log.data(), O_RDONLY | O_CLOEXEC);
    if (in < 0) {
        return;
    }
    const char* slash = std::strrchr(g_log.data(), '/');
    Text path;
    path.add(folder);
    path.add('/');
    path.add(slash != nullptr ? slash + 1 : g_log.data());
    const int out = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (out >= 0) {
        std::array<char, 8192> buffer{};
        for (;;) {
            const ssize_t got = ::read(in, buffer.data(), buffer.size());
            if (got <= 0) {
                break;
            }
            writeAll(out, std::string_view(buffer.data(), static_cast<std::size_t>(got)));
        }
        ::close(out);
    }
    ::close(in);
}

void writeReport(int signal, const siginfo_t* info, void* context) noexcept
{
    timespec now{};
    ::clock_gettime(CLOCK_REALTIME, &now);
    const std::int64_t seconds = now.tv_sec;
    const Civil date = civil(seconds / 86400);
    const auto inDay = static_cast<std::uint64_t>(seconds % 86400);
    const auto milliseconds = static_cast<std::uint64_t>(now.tv_nsec / 1'000'000);

    Text name;
    name.add(kCrashFolderPrefix);
    name.number(static_cast<std::uint64_t>(date.year), 10, 4);
    name.number(date.month, 10, 2);
    name.number(date.day, 10, 2);
    name.add('-');
    name.number(inDay / 3600, 10, 2);
    name.number(inDay / 60 % 60, 10, 2);
    name.number(inDay % 60, 10, 2);
    name.add('-');
    name.number(milliseconds, 10, 3);

    Text folder;
    folder.add(g_directory.data());
    folder.add('/');
    folder.add(name.view());
    if (::mkdir(folder.c_str(), 0700) != 0 && errno != EEXIST) {
        return;
    }
    copyLog(folder.c_str());

    Text text;
    const BuildInfo build = buildInfo();
    text.add("version ");
    text.add(build.version);
    text.add(" (");
    text.add(build.revision);
    text.add(")\ntime ");
    text.number(static_cast<std::uint64_t>(date.year), 10, 4);
    text.add('-');
    text.number(date.month, 10, 2);
    text.add('-');
    text.number(date.day, 10, 2);
    text.add(' ');
    text.number(inDay / 3600, 10, 2);
    text.add(':');
    text.number(inDay / 60 % 60, 10, 2);
    text.add(':');
    text.number(inDay % 60, 10, 2);
    text.add('.');
    text.number(milliseconds, 10, 3);
    text.add(" UTC\nsignal ");
    text.number(static_cast<std::uint64_t>(signal), 10, 0);
    if (const std::string_view named = signalName(signal); !named.empty()) {
        text.add(' ');
        text.add(named);
    }
    if (info != nullptr && (signal == SIGSEGV || signal == SIGBUS)) {
        text.add(" at 0x");
        text.number(reinterpret_cast<std::uintptr_t>(info->si_addr), 16, 16);
    }
    const std::uintptr_t address = programCounter(context);
    text.add("\naddress 0x");
    text.number(address, 16, 16);
    Dl_info found{};
    if (address != 0 && ::dladdr(reinterpret_cast<void*>(address), &found) != 0 && found.dli_fname != nullptr) {
        const char* slash = std::strrchr(found.dli_fname, '/');
        text.add(' ');
        text.add(slash != nullptr ? slash + 1 : found.dli_fname);
        text.add("+0x");
        text.number(address - reinterpret_cast<std::uintptr_t>(found.dli_fbase), 16, 0);
    }
    text.add("\nthread ");
    text.number(threadNumber(), 10, 0);
    text.add('\n');

    Text file;
    file.add(folder.view());
    file.add("/crash.txt");
    const int out = ::open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (out >= 0) {
        writeAll(out, text.view());
        ::close(out);
    }
}

void onSignal(int signal, siginfo_t* info, void* context)
{
    bool first = false;
    if (g_crashing.compare_exchange_strong(first, true)) {
        writeReport(signal, info, context);
    }
    // Back to the default action, and again: the fault happens anew, or the signal is raised once more.
    struct sigaction usual{};
    usual.sa_handler = SIG_DFL;
    sigemptyset(&usual.sa_mask);
    ::sigaction(signal, &usual, nullptr);
    if (info == nullptr || info->si_code <= 0 || signal == SIGABRT) {
        ::raise(signal);
    }
}

bool copyPath(std::array<char, kMostPath>& into, const std::filesystem::path& path) noexcept
{
    const std::string& text = path.native();
    if (text.size() + 1 > into.size()) {
        return false;
    }
    std::memcpy(into.data(), text.c_str(), text.size() + 1);
    return true;
}

}

bool installCrashHandler(const std::filesystem::path& directory, const std::filesystem::path& log)
{
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error || !copyPath(g_directory, std::filesystem::absolute(directory, error)) ||
        !copyPath(g_log, log.empty() ? log : std::filesystem::absolute(log, error))) {
        return false;
    }
    static std::array<char, 64 * 1024> stack{};
    stack_t alternate{};
    alternate.ss_sp = stack.data();
    alternate.ss_size = stack.size();
    ::sigaltstack(&alternate, nullptr);
    struct sigaction action{};
    action.sa_sigaction = &onSignal;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);
    bool installed = true;
    for (const int signal : kSignals) {
        installed = ::sigaction(signal, &action, nullptr) == 0 && installed;
    }
    return installed;
}

void crashOnPurpose() noexcept
{
    volatile int* nowhere = nullptr;
    *nowhere = 0;
    std::_Exit(EXIT_FAILURE);
}

}
