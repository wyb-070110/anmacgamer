#include "ScriptProject.h"

#include <gs/input/Profile.h>
#include <gs/platform/AtomicFile.h>
#include <gs/script/ApiFiles.h>

#include <mach-o/dyld.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <vector>

#include <fstream>
#include <iterator>
#include <system_error>

namespace gs::cli {

namespace fs = std::filesystem;

namespace {

std::string environmentText(const char* name)
{
    const char* value = std::getenv(name);
    return value != nullptr ? std::string(value) : std::string();
}

}

std::string utf8(const fs::path& path)
{
    const std::u8string text = path.generic_u8string();
    return {reinterpret_cast<const char*>(text.data()), text.size()};
}

fs::path pathOf(std::string_view text)
{
    return fs::path(std::u8string(reinterpret_cast<const char8_t*>(text.data()), text.size()));
}

std::optional<std::string> readText(const fs::path& file)
{
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    return std::string{std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

bool writeText(const fs::path& file, std::string_view contents)
{
    std::error_code error;
    fs::create_directories(file.parent_path(), error);
    return !error && !platform::writeFileAtomically(file, contents);
}

bool isProfileFolder(const fs::path& folder)
{
    std::error_code error;
    return fs::is_regular_file(folder / pathOf(kProfileFile), error);
}

std::optional<std::string> entryOf(const fs::path& folder)
{
    const bool profile = isProfileFolder(folder);
    const std::string_view candidates[] = {profile ? "logic.ts" : "src/main.ts", profile ? "logic.js" : "src/main.js",
                                           "task.ts", "component.ts"};
    for (const std::string_view candidate : candidates) {
        std::error_code error;
        if (fs::is_regular_file(folder / pathOf(candidate), error)) {
            return std::string(candidate);
        }
    }
    return std::nullopt;
}

Expected<ProjectProfile, std::vector<std::string>> loadProjectProfile(const fs::path& file)
{
    const std::optional<std::string> text = readText(file);
    if (!text) {
        return makeUnexpected(std::vector<std::string>{"$: the profile cannot be read"});
    }
    const input::LoadResult loaded = input::loadProfile(*text);
    std::vector<std::string> problems;
    for (const input::Diagnostic& diagnostic : loaded.diagnostics) {
        if (diagnostic.severity == input::Severity::Error) {
            problems.push_back(diagnostic.detail.empty() ? diagnostic.path : diagnostic.path + ": " + diagnostic.detail);
        }
    }
    if (!problems.empty()) {
        return makeUnexpected(std::move(problems));
    }
    ProjectProfile profile;
    for (const input::Control& control : loaded.profile.controls) {
        profile.controls.push_back({control.id, control.name,
                                    std::string(input::kControlTypeNames[control.kind.index()]), control.enabled,
                                    input::controlSettings(control)});
    }
    for (const input::Variable& variable : loaded.profile.vars) {
        profile.states.push_back({variable.id, variable.name,
                                  std::visit([](const auto& each) { return script::VarValue(each); }, variable.initial)});
    }
    return profile;
}

std::string apiDeclarations()
{
    for (const script::ApiFile& file : script::apiFiles()) {
        if (file.path == kApiTypesFile) {
            return file.contents;
        }
    }
    return {};
}

fs::path programFolder()
{
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size + 1, '\0');
    if (size == 0 || _NSGetExecutablePath(buffer.data(), &size) != 0) {
        return {};
    }
    std::error_code error;
    const fs::path program = fs::canonical(buffer.data(), error);
    return (error ? fs::path(buffer.data()) : program).parent_path();
}

fs::path dataFolder()
{
    if (const std::string overridden = environmentText("GS_DATA_DIR"); !overridden.empty()) {
        return pathOf(overridden);
    }
    std::error_code error;
    if (fs::exists(programFolder() / "portable.flag", error)) {
        return programFolder() / "data";
    }
    const std::string home = environmentText("HOME");
    if (home.empty()) {
        return {};
    }
    const fs::path support = pathOf(home) / "Library" / "Application Support";
    // Before the app first runs as anmacgamer (and moves the folder), it is still called GameScrcpy.
    return !fs::exists(support / "anmacgamer", error) && fs::is_directory(support / "GameScrcpy", error)
               ? support / "GameScrcpy"
               : support / "anmacgamer";
}

void writeTo(Stream stream, std::string_view text)
{
    const int fd = stream == Stream::Out ? STDOUT_FILENO : STDERR_FILENO;
    while (!text.empty()) {
        const ssize_t written = ::write(fd, text.data(), text.size());
        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written <= 0) {
            return;
        }
        text.remove_prefix(static_cast<std::size_t>(written));
    }
}

}
