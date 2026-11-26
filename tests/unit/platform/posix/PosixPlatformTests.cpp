// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// The POSIX platform backend: atomic files, the running marker and word spacing for read text.
#include "gs/platform/AtomicFile.h"
#if defined(__APPLE__)
#include "gs/platform/InputCapture.h"
#endif
#include "gs/platform/QuitSignal.h"
#include "gs/platform/RunningMarker.h"
#include "gs/platform/TextReader.h"

#include <doctest/doctest.h>

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

using namespace gs::platform;

namespace {

class Folder
{
public:
    Folder()
    {
        std::string pattern = (std::filesystem::temp_directory_path() / "gs-posix-XXXXXX").string();
        REQUIRE(::mkdtemp(pattern.data()) != nullptr);
        m_path = pattern;
    }
    ~Folder()
    {
        std::error_code error;
        std::filesystem::remove_all(m_path, error);
    }
    Folder(const Folder&) = delete;
    Folder& operator=(const Folder&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const { return m_path; }

private:
    std::filesystem::path m_path;
};

std::string contentsOf(const std::filesystem::path& file)
{
    std::ifstream in(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

}

TEST_CASE("an atomic write creates or replaces the file and leaves no temporary")
{
    const Folder folder;
    const auto file = folder.path() / "settings.json";
    REQUIRE_FALSE(writeFileAtomically(file, "first"));
    CHECK(contentsOf(file) == "first");
    REQUIRE_FALSE(writeFileAtomically(file, "second, longer"));
    CHECK(contentsOf(file) == "second, longer");
    CHECK_FALSE(std::filesystem::exists(folder.path() / "settings.json.tmp"));
}

TEST_CASE("a temporary left by an interrupted write is overwritten")
{
    const Folder folder;
    const auto file = folder.path() / "a.json";
    std::ofstream(folder.path() / "a.json.tmp") << "half written";
    REQUIRE_FALSE(writeFileAtomically(file, "whole"));
    CHECK(contentsOf(file) == "whole");
    CHECK_FALSE(std::filesystem::exists(folder.path() / "a.json.tmp"));
}

TEST_CASE("an atomic write into a missing directory fails and creates nothing")
{
    const Folder folder;
    const auto file = folder.path() / "missing" / "a.json";
    const std::error_code error = writeFileAtomically(file, "x");
    CHECK(error);
    CHECK(fileMissing(error));
    CHECK_FALSE(std::filesystem::exists(folder.path() / "missing"));
}

TEST_CASE("reading tells a missing and an oversized file apart and reads a whole one")
{
    const Folder folder;
    std::string contents;
    const std::error_code missing = readFile(folder.path() / "none", 100, contents);
    CHECK(fileMissing(missing));
    CHECK_FALSE(fileBusy(missing));
    REQUIRE_FALSE(writeFileAtomically(folder.path() / "big", std::string(200, 'x')));
    CHECK(readFile(folder.path() / "big", 100, contents) == std::make_error_code(std::errc::file_too_large));
    REQUIRE_FALSE(readFile(folder.path() / "big", 200, contents));
    CHECK(contents == std::string(200, 'x'));
}

TEST_CASE("replacing moves a finished file over the target, and flushing works on files and folders")
{
    const Folder folder;
    const auto from = folder.path() / "from";
    const auto to = folder.path() / "to";
    std::ofstream(from) << "new";
    std::ofstream(to) << "old";
    REQUIRE_FALSE(replaceFile(from, to));
    CHECK(contentsOf(to) == "new");
    CHECK_FALSE(std::filesystem::exists(from));
    CHECK_FALSE(flushFile(to));
    CHECK_FALSE(flushTree(folder.path()));
    CHECK(fileMissing(flushFile(folder.path() / "none")));
}

TEST_CASE("removing a file works, and a missing file counts as removed")
{
    const Folder folder;
    const auto file = folder.path() / "gone";
    std::ofstream(file) << "x";
    CHECK_FALSE(removeFile(file));
    CHECK_FALSE(std::filesystem::exists(file));
    CHECK_FALSE(removeFile(file));
}

TEST_CASE("a writer killed at any moment leaves the file whole")
{
    const Folder folder;
    const auto file = folder.path() / "state.json";
    const std::string before(64 * 1024, 'a');
    const std::string after(64 * 1024, 'b');
    REQUIRE_FALSE(writeFileAtomically(file, before));
    for (int attempt = 0; attempt < 20; ++attempt) {
        const pid_t child = ::fork();
        REQUIRE(child >= 0);
        if (child == 0) {
            for (;;) {
                static_cast<void>(writeFileAtomically(file, after));
                static_cast<void>(writeFileAtomically(file, before));
            }
        }
        ::usleep(static_cast<useconds_t>(1000 + attempt * 500)); // source-rule-allow R9: let the child get into a write before it is killed
        ::kill(child, SIGKILL);
        int status = 0;
        ::waitpid(child, &status, 0);
        const std::string seen = contentsOf(file);
        CHECK((seen == before || seen == after));
    }
}

TEST_CASE("the running marker is seen while held and gone after")
{
    const std::wstring name = L"GameScrcpyTest-" + std::to_wstring(::getpid());
    CHECK_FALSE(runningMarkerPresent(name));
    {
        const RunningMarker marker(name);
        REQUIRE(marker.held());
        const RunningMarker second(name);
        CHECK(second.held());
        const pid_t child = ::fork();
        REQUIRE(child >= 0);
        if (child == 0) {
            ::_exit(runningMarkerPresent(name) ? 0 : 1);
        }
        int status = 0;
        ::waitpid(child, &status, 0);
        CHECK((WIFEXITED(status) && WEXITSTATUS(status) == 0));
    }
    CHECK_FALSE(runningMarkerPresent(name));
}

TEST_CASE("words are spaced only between two letters that are not Chinese, Japanese or full width")
{
    CHECK(joinsWithSpace(U'a', U'1'));
    CHECK(joinsWithSpace(U'%', U'B'));
    CHECK(joinsWithSpace(U'한', U'국'));
    CHECK_FALSE(joinsWithSpace(U'电', U'池'));
    CHECK_FALSE(joinsWithSpace(U'a', U'池'));
    CHECK_FALSE(joinsWithSpace(U'ア', U'イ'));
    CHECK_FALSE(joinsWithSpace(U'Ａ', U'b'));
    CHECK_FALSE(joinsWithSpace(0, U'b'));
}

TEST_CASE("quit signals wake the self-pipe for SIGTERM, SIGINT and SIGHUP even alongside InputCapture")
{
    for (const int sig : {SIGTERM, SIGINT, SIGHUP}) {
        std::array<int, 2> ready{-1, -1};
        REQUIRE(::pipe(ready.data()) == 0);
        const pid_t child = ::fork();
        REQUIRE(child >= 0);
        if (child == 0) {
            ::close(ready[0]);
            sigset_t blocked{};
            sigemptyset(&blocked);
            sigaddset(&blocked, sig);
            ::pthread_sigmask(SIG_BLOCK, &blocked, nullptr);

            QuitSignal watch;
            if (watch.descriptor() < 0) {
                ::_exit(2);
            }
#if defined(__APPLE__)
            struct QuietHandler final : InputHandler {
                void key(const RawKey&, gs::Microseconds) override {}
                void mouse(const RawMouse&, gs::Microseconds) override {}
                void gamepad(const PadState&, gs::Microseconds) override {}
                void wake(gs::Microseconds) override {}
                [[nodiscard]] std::optional<gs::Microseconds> nextWakeup() const override { return std::nullopt; }
            } quiet;
            InputCapture capture(quiet);
#endif
            const char go = 'r';
            static_cast<void>(::write(ready[1], &go, 1));
            ::close(ready[1]);

            pollfd pfd{.fd = watch.descriptor(), .events = POLLIN, .revents = 0};
            int polled = 0;
            do {
                polled = ::poll(&pfd, 1, 5000);
            } while (polled < 0 && errno == EINTR);
            if (polled <= 0) {
                ::_exit(3);
            }
            const std::optional<int> got = watch.take();
            ::_exit(got && *got == sig && !quitSignalName(*got).empty() ? 0 : 4);
        }
        ::close(ready[1]);
        char go = 0;
        REQUIRE(::read(ready[0], &go, 1) == 1);
        ::close(ready[0]);
        REQUIRE(::kill(child, sig) == 0);
        int status = 0;
        REQUIRE(::waitpid(child, &status, 0) == child);
        CHECK((WIFEXITED(status) && WEXITSTATUS(status) == 0));
    }
}

TEST_CASE("a second termination signal while quitting exits at once")
{
    std::array<int, 2> ready{-1, -1};
    std::array<int, 2> hold{-1, -1};
    REQUIRE(::pipe(ready.data()) == 0);
    REQUIRE(::pipe(hold.data()) == 0);
    const pid_t child = ::fork();
    REQUIRE(child >= 0);
    if (child == 0) {
        ::close(ready[0]);
        ::close(hold[1]);
        QuitSignal watch;
        const char r = 'r';
        static_cast<void>(::write(ready[1], &r, 1));

        pollfd pfd{.fd = watch.descriptor(), .events = POLLIN, .revents = 0};
        int polled = 0;
        do {
            polled = ::poll(&pfd, 1, 5000);
        } while (polled < 0 && errno == EINTR);
        if (polled <= 0 || !watch.take()) {
            ::_exit(2);
        }
        const char q = 'q';
        static_cast<void>(::write(ready[1], &q, 1));
        ::close(ready[1]);

        char never = 0;
        static_cast<void>(::read(hold[0], &never, 1));
        ::_exit(3);
    }
    ::close(ready[1]);
    ::close(hold[0]);
    char mark = 0;
    REQUIRE(::read(ready[0], &mark, 1) == 1);
    REQUIRE(::kill(child, SIGTERM) == 0);
    REQUIRE(::read(ready[0], &mark, 1) == 1);
    CHECK(mark == 'q');
    REQUIRE(::kill(child, SIGTERM) == 0);
    int status = 0;
    REQUIRE(::waitpid(child, &status, 0) == child);
    ::close(ready[0]);
    ::close(hold[1]);
    CHECK((WIFSIGNALED(status) && WTERMSIG(status) == SIGTERM));
}
