// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// gs-exthost off Windows: the test DLL as a macOS library in gs-exthost, through the parts of the port the mock host
// tests do not reach: shared memory sent to the child (a frame tap) and taken from it (a picture source), a crash
// with its report and restart, and a restricted DLL in the macOS sandbox.
#include <gs/exthost/Hosted.h>
#include <gs/exthost/Media.h>
#include <gs/exthost/Module.h>

#include <gamescrcpy_ext.h>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <mutex>
#include <optional>
#include <random>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

namespace exthost = gs::exthost;
namespace fs = std::filesystem;
using namespace std::chrono_literals;

namespace {

const fs::path kTestDll(GS_TEST_NATIVE_DLL);
const fs::path kProgram(GS_EXTHOST_PROGRAM);

class Scratch
{
public:
    Scratch()
    {
        std::random_device random;
        m_path = fs::temp_directory_path() / ("gs-exthost-tests-" + std::to_string(random()));
        fs::create_directories(m_path);
        m_path = fs::canonical(m_path);
    }
    ~Scratch()
    {
        std::error_code ignored;
        fs::remove_all(m_path, ignored);
    }
    Scratch(const Scratch&) = delete;
    Scratch& operator=(const Scratch&) = delete;

    [[nodiscard]] fs::path operator/(const fs::path& more) const { return m_path / more; }

private:
    fs::path m_path;
};

void writeFile(const fs::path& file, std::string_view text)
{
    fs::create_directories(file.parent_path());
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
}

std::string readFile(const fs::path& file)
{
    std::ifstream in(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

exthost::Value numberValue(double value)
{
    exthost::Value made;
    made.type = exthost::ValueType::Number;
    made.number = value;
    return made;
}

exthost::Value textValue(exthost::ValueType type, std::string text)
{
    exthost::Value made;
    made.type = type;
    made.text = std::move(text);
    return made;
}

exthost::Value stringValue(std::string text)
{
    return textValue(exthost::ValueType::String, std::move(text));
}

exthost::CallResult callNode(exthost::Module& module, std::string_view name, std::vector<exthost::Value> values = {})
{
    const auto& nodes = module.info().nodes;
    const auto found = std::ranges::find_if(nodes, [name](const auto& node) { return node.name == name; });
    REQUIRE(found != nodes.end());
    return module.call(static_cast<std::size_t>(found - nodes.begin()), values);
}

bool eventually(const std::function<bool()>& done)
{
    const auto deadline = std::chrono::steady_clock::now() + 20s;
    while (!done()) {
        if (std::chrono::steady_clock::now() > deadline) {
            return false;
        }
        std::this_thread::sleep_for(10ms);
    }
    return true;
}

exthost::HostedOptions hostedOptions(const Scratch& scratch)
{
    exthost::HostedOptions options;
    options.program = kProgram;
    options.dll = kTestDll;
    options.store = scratch / "store/gstest.native.json";
    options.granted = {"notify"};
    return options;
}

std::unique_ptr<exthost::HostedModule> started(exthost::HostedOptions options)
{
    std::string problem;
    std::unique_ptr<exthost::HostedModule> module = exthost::HostedModule::start(std::move(options), problem);
    REQUIRE_MESSAGE(module != nullptr, problem);
    REQUIRE_MESSAGE(module->waitReady(20s), "gs-exthost did not get the test DLL ready");
    return module;
}

void crashOnce(exthost::HostedModule& module, int expected)
{
    const exthost::CallResult crashed = callNode(module, "crash");
    CHECK(crashed.status != exthost::CallStatus::Ok);
    REQUIRE(eventually([&] { return module.crashes() == expected; }));
}

}

TEST_CASE("a node runs in gs-exthost on macOS, with the user's environment when the DLL has full rights")
{
    Scratch scratch;
    const std::unique_ptr<exthost::HostedModule> module = started(hostedOptions(scratch));
    const exthost::CallResult added = callNode(*module, "add", {numberValue(2), numberValue(3)});
    REQUIRE_MESSAGE(added.ok(), added.problem);
    CHECK(added.value.number == 5.0);
    ::setenv("GS_TEST_SEEN", "yes", 1);
    const std::unique_ptr<exthost::HostedModule> fresh = started(hostedOptions(scratch));
    CHECK(callNode(*fresh, "environment", {stringValue("GS_TEST_SEEN")}).value.text == "yes");
    ::unsetenv("GS_TEST_SEEN");
}

TEST_CASE("a frame tap in gs-exthost gets its frames through shared memory the app sends over the bridge")
{
    Scratch scratch;
    const std::unique_ptr<exthost::HostedModule> module = started(hostedOptions(scratch));
    REQUIRE(module->info().frameTaps.size() == 1);
    REQUIRE(callNode(*module, "tapReset").ok());
    constexpr std::uint32_t kStride = 64 * 4 + 16;
    std::vector<std::uint8_t> pixels(std::size_t{kStride} * 36, 0);
    for (std::uint32_t y = 0; y < 36; ++y) {
        for (std::uint32_t x = 0; x < 64; ++x) {
            std::uint8_t* pixel = pixels.data() + std::size_t{y} * kStride + std::size_t{4} * x;
            pixel[0] = 10;
            pixel[1] = 20;
            pixel[2] = 30;
            pixel[3] = 255;
        }
    }
    std::mutex mutex;
    std::condition_variable changed;
    std::vector<std::vector<exthost::Shape>> overlays;
    const exthost::TapEvents events{.overlay =
                                        [&](std::vector<exthost::Shape> shapes) {
                                            {
                                                const std::scoped_lock lock(mutex);
                                                overlays.push_back(std::move(shapes));
                                            }
                                            changed.notify_all();
                                        },
                                    .failed = {}};
    const std::uint64_t tap =
        module->openTap(0, {.device = "phone-1", .width = 64, .height = 36, .rotation = 1}, events);
    REQUIRE(tap != 0);
    CHECK(module->offerFrame(tap, {.pixels = pixels.data(), .stride = kStride, .width = 64, .height = 36, .timeUs = 1000}));
    {
        std::unique_lock lock(mutex);
        REQUIRE(changed.wait_for(lock, 20s, [&] { return !overlays.empty(); }));
        REQUIRE(overlays[0].size() == 1);
        CHECK(overlays[0][0].text == "box");
    }
    const exthost::CallResult stats = callNode(*module, "tapStats");
    REQUIRE_MESSAGE(stats.ok(), stats.problem);
    const nlohmann::json seen = nlohmann::json::parse(stats.value.text);
    CHECK(seen["opened"] == 1);
    CHECK(seen["frameWidth"] == 64);
    CHECK(seen["first"] == nlohmann::json::array({10, 20, 30, 255}));
    module->closeTap(tap);
}

TEST_CASE("a picture source in gs-exthost hands its frames through shared memory the app takes over the bridge")
{
    Scratch scratch;
    const std::unique_ptr<exthost::HostedModule> module = started(hostedOptions(scratch));
    REQUIRE_FALSE(module->info().videoSources.empty());
    const std::shared_ptr<exthost::MediaStream> bars =
        module->openSource(exthost::MediaKind::Video, 0, "bars", R"({"frames":3,"pause":20,"width":64,"height":48})");
    REQUIRE(bars != nullptr);
    std::stop_source stop;
    std::jthread watchdog([&stop](const std::stop_token& token) {
        std::mutex mutex;
        std::condition_variable_any idle;
        std::unique_lock lock(mutex);
        static_cast<void>(idle.wait_for(lock, token, 20s, [] { return false; }));
        stop.request_stop();
    });
    int frames = 0;
    std::array<std::uint8_t, 4> pixel{};
    std::optional<std::string> ended;
    while (bars->wait(stop.get_token())) {
        if (const std::optional<exthost::FrameView> frame = bars->frame()) {
            ++frames;
            CHECK(frame->width == 64);
            const std::uint8_t* at = frame->data + std::size_t{7} * frame->stride + std::size_t{5} * 4;
            pixel = {at[0], at[1], at[2], at[3]};
        }
        if ((ended = bars->ended())) {
            break;
        }
    }
    CHECK(frames >= 1);
    CHECK(pixel == std::array<std::uint8_t, 4>{5, 7, 80, 255});
    CHECK(ended == std::optional<std::string>("done"));
    module->closeSource(bars->id());
}

TEST_CASE("a crash of gs-exthost leaves crash.txt and the log in a crash folder, reads as an access violation, and "
          "gs-exthost starts again")
{
    Scratch scratch;
    writeFile(scratch / "gamescrcpy.log", "a line of the program's log\n");
    std::mutex mutex;
    std::vector<std::uint32_t> codes;
    std::atomic<int> restarts{0};
    exthost::HostedOptions options = hostedOptions(scratch);
    options.crashes = scratch / "crashes with spaces" / "gstest.native";
    options.logFile = scratch / "gamescrcpy.log";
    options.crashed = [&mutex, &codes](std::uint32_t code) {
        const std::scoped_lock lock(mutex);
        codes.push_back(code);
    };
    options.restarted = [&restarts] { ++restarts; };
    const std::unique_ptr<exthost::HostedModule> module = started(std::move(options));
    crashOnce(*module, 1);
    REQUIRE(eventually([&] {
        const std::scoped_lock lock(mutex);
        return codes.size() == 1;
    }));
    CHECK(codes[0] == 0xC0000005U);
    std::vector<fs::path> folders;
    for (const fs::directory_entry& entry : fs::directory_iterator(scratch / "crashes with spaces" / "gstest.native")) {
        folders.push_back(entry.path());
    }
    REQUIRE(folders.size() == 1);
    CHECK(folders[0].filename().string().starts_with("crash-"));
    CHECK(folders[0].filename().string().size() == 25);
    const std::string summary = readFile(folders[0] / "crash.txt");
    CHECK((summary.find("signal 11 SIGSEGV") != std::string::npos || summary.find("signal 10 SIGBUS") != std::string::npos));
    CHECK(summary.find("libgs_test_native") != std::string::npos);
    CHECK(readFile(folders[0] / "gamescrcpy.log") == "a line of the program's log\n");

    REQUIRE(module->waitReady(20s));
    CHECK(restarts == 1);
    CHECK(callNode(*module, "add", {numberValue(1), numberValue(1)}).value.number == 2.0);
}

// Run by the next test in a process of its own: hosts the test DLL with the gs-exthost copy it is given, then waits
// to be killed.
TEST_CASE("(helper) hold a DLL in gs-exthost until this process is killed")
{
    const char* program = std::getenv("GS_EXTHOST_TEST_HOLD");
    if (program == nullptr) {
        return;
    }
    Scratch scratch;
    exthost::HostedOptions options = hostedOptions(scratch);
    options.program = program;
#if defined(__APPLE__)
    // Restricted: no --parent to watch, so only the bridge can tell gs-exthost that the app is gone.
    const fs::path folder = scratch / "gstest.orphan/1.0.0";
    fs::create_directories(folder);
    fs::copy_file(kTestDll, folder / "native.dylib");
    options.dll = folder / "native.dylib";
    options.container = "gstest.orphan";
    options.readable = folder;
#endif
    const std::unique_ptr<exthost::HostedModule> module = started(std::move(options));
    std::this_thread::sleep_for(60s);
}

TEST_CASE("gs-exthost ends by itself when GameScrcpy is killed, as the job object ends it on Windows")
{
    Scratch scratch;
    const fs::path copy = scratch / "orphan" / "gs-exthost-orphan";
    fs::create_directories(copy.parent_path());
    fs::copy_file(kProgram, copy);
    const auto running = [&copy] {
        const std::string command = "pgrep -f '" + copy.string() + "' >/dev/null";
        return std::system(command.c_str()) == 0;
    };
    std::vector<std::string> words{GS_EXTHOST_TESTS_PROGRAM, "--test-case=(helper)*"};
    std::vector<char*> argv;
    for (std::string& word : words) {
        argv.push_back(word.data());
    }
    argv.push_back(nullptr);
    std::string hold = "GS_EXTHOST_TEST_HOLD=" + copy.string();
    std::string home = std::string("HOME=") + (std::getenv("HOME") != nullptr ? std::getenv("HOME") : "/tmp");
    std::array<char*, 3> envp{hold.data(), home.data(), nullptr};
    pid_t helper = 0;
    REQUIRE(::posix_spawn(&helper, words[0].c_str(), nullptr, nullptr, argv.data(), envp.data()) == 0);
    REQUIRE_MESSAGE(eventually(running), "the helper did not start gs-exthost");
    ::kill(helper, SIGKILL);
    int status = 0;
    ::waitpid(helper, &status, 0);
    CHECK(WIFSIGNALED(status));
    CHECK_MESSAGE(eventually([&running] { return !running(); }), "gs-exthost outlived the app that started it");
    static_cast<void>(exthost::forgetContainer("gstest.orphan"));
}

#if defined(__APPLE__)

TEST_CASE("a restricted DLL runs in gs-exthost inside the macOS sandbox: its nodes and store work, the user's files and "
          "environment are out of reach, and a file the app shares becomes readable")
{
    Scratch scratch;
    const fs::path folder = scratch / "gstest.restricted/1.0.0";
    fs::create_directories(folder);
    fs::copy_file(kTestDll, folder / "native.dylib");
    writeFile(scratch / "private/notes.txt", "not for the extension");
    writeFile(scratch / "shared/picture.txt", "for the extension");
    ::setenv("GS_TEST_SECRET", "leak", 1);
    exthost::HostedOptions options = hostedOptions(scratch);
    options.dll = folder / "native.dylib";
    options.store = scratch / "store/gstest.restricted.json";
    options.container = "gstest.restricted";
    options.readable = folder;
    const std::unique_ptr<exthost::HostedModule> hosted = started(std::move(options));

    const exthost::CallResult added = callNode(*hosted, "add", {numberValue(2), numberValue(3)});
    REQUIRE_MESSAGE(added.ok(), added.problem);
    CHECK(added.value.number == 5.0);

    const exthost::CallResult outside = callNode(*hosted, "openFile", {stringValue((scratch / "private/notes.txt").string())});
    REQUIRE_MESSAGE(outside.ok(), outside.problem);
    CHECK(outside.value.number == 5.0);
    const exthost::CallResult own = callNode(*hosted, "openFile", {stringValue((folder / "native.dylib").string())});
    REQUIRE_MESSAGE(own.ok(), own.problem);
    CHECK(own.value.number == 0.0);
    CHECK(callNode(*hosted, "environment", {stringValue("GS_TEST_SECRET")}).value.text.empty());
    ::unsetenv("GS_TEST_SECRET");

    const exthost::CallResult kept = callNode(
        *hosted, "remember", {stringValue("board"), textValue(exthost::ValueType::Json, R"({"port":"cu.usbserial"})")});
    REQUIRE_MESSAGE(kept.ok(), kept.problem);
    CHECK(kept.value.flag);
    CHECK(nlohmann::json::parse(callNode(*hosted, "recall", {stringValue("board")}).value.text) ==
          nlohmann::json{{"port", "cu.usbserial"}});
    CHECK(nlohmann::json::parse(readFile(scratch / "store/gstest.restricted.json")).at("board").at("port") ==
          "cu.usbserial");

    const fs::path shared = scratch / "shared/picture.txt";
    CHECK(callNode(*hosted, "openFile", {stringValue(shared.string())}).value.number == 5.0);
    std::string problem;
    REQUIRE_MESSAGE(exthost::shareWithContainer("gstest.restricted", shared, problem), problem);
    CHECK(eventually([&] { return callNode(*hosted, "openFile", {stringValue(shared.string())}).value.number == 0.0; }));
    CHECK(callNode(*hosted, "openFile", {stringValue((scratch / "private/notes.txt").string())}).value.number == 5.0);

    exthost::HostedOptions trusted = hostedOptions(scratch);
    const std::unique_ptr<exthost::HostedModule> full = started(std::move(trusted));
    CHECK(callNode(*full, "openFile", {stringValue((scratch / "private/notes.txt").string())}).value.number == 0.0);
}

TEST_CASE("forgetting a container removes its folder, and forgetting one that never ran is harmless")
{
    Scratch scratch;
    const fs::path folder = scratch / "gstest.forgotten/1.0.0";
    fs::create_directories(folder);
    fs::copy_file(kTestDll, folder / "native.dylib");
    exthost::HostedOptions options = hostedOptions(scratch);
    options.dll = folder / "native.dylib";
    options.store = scratch / "store/gstest.forgotten.json";
    options.container = "gstest.forgotten";
    options.readable = folder;
    {
        const std::unique_ptr<exthost::HostedModule> hosted = started(std::move(options));
        CHECK(callNode(*hosted, "add", {numberValue(1), numberValue(1)}).value.number == 2.0);
    }
    CHECK(exthost::forgetContainer("gstest.forgotten"));
    CHECK(exthost::forgetContainer("gstest.never-ran"));
}

#endif
