// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Mach-O libraries for macOS native extensions: reading them, sorting their imports, and the package checks.
#include "gs/ext/Archive.h"
#include "gs/ext/Manifest.h"
#include "gs/ext/NativeCheck.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace gs::ext;

namespace {

std::string bigEndian(std::uint32_t value)
{
    return {static_cast<char>(value >> 24), static_cast<char>(value >> 16), static_cast<char>(value >> 8),
            static_cast<char>(value)};
}

bool hasCode(const std::vector<Diagnostic>& found, Code code)
{
    return std::ranges::any_of(found, [code](const Diagnostic& diagnostic) { return diagnostic.code == code; });
}

#if defined(GS_TEST_MACHO_CLEAN)

std::string fileBytes(const char* path)
{
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

bool imports(const PeImports& read, std::string_view module, std::string_view name)
{
    return std::ranges::any_of(read.functions, [&](const ImportedFunction& function) {
        return function.module == module && function.name == name && function.macho;
    });
}

bool uses(const std::vector<CapabilityUse>& found, Capability capability)
{
    return std::ranges::any_of(found, [capability](const CapabilityUse& use) { return use.capability == capability; });
}

// A universal file holding one x86_64 placeholder slice that is not a library, then the given thin image.
std::string universal(const std::string& thin)
{
    const std::string junk(64, '\0');
    std::string out = bigEndian(0xCAFEBABE) + bigEndian(2);
    out += bigEndian(0x01000007) + bigEndian(3) + bigEndian(4096) + bigEndian(64) + bigEndian(12);
    out += bigEndian(0x0100000C) + bigEndian(0) + bigEndian(8192) + bigEndian(static_cast<std::uint32_t>(thin.size())) +
           bigEndian(14);
    out.resize(4096, '\0');
    out += junk;
    out.resize(8192, '\0');
    return out + thin;
}

PackageContents package(std::map<std::string, std::string> files)
{
    PackageContents contents;
    for (const auto& [path, bytes] : files) {
        contents.files.push_back({path, bytes.size()});
    }
    contents.read = [files](const std::string& path) -> std::optional<std::string> {
        const auto found = files.find(path);
        return found != files.end() ? std::optional(found->second) : std::nullopt;
    };
    return contents;
}

Manifest nativeManifest(TextMap dylibs, std::string dylib = {}, std::string dll = {})
{
    Manifest manifest;
    manifest.id = "example.macho";
    NativeContribution native;
    native.dll = std::move(dll);
    native.dylib = std::move(dylib);
    native.dylibs = std::move(dylibs);
    manifest.contributes.native = std::move(native);
    return manifest;
}

#endif

}

TEST_CASE("macOS loads the manifest's dylib, by processor or one for all, and Windows its dll, never the other")
{
    CHECK(architectureMachine("macos-arm64") == kMachineMacArm64);
    CHECK(machineName(kMachineMacArm64) == "macos-arm64");
    CHECK(macArchitecture("macos-x64"));
    CHECK(macArchitecture("macos"));
    CHECK_FALSE(macArchitecture("arm64"));

    NativeContribution windows;
    windows.dll = "native.dll";
    CHECK(nativeDllFor(windows, "x64") == "native.dll");
    CHECK(nativeDllFor(windows, "macos-arm64").empty());

    NativeContribution both;
    both.dlls = {{"arm64", "bin/arm64/native.dll"}};
    both.dylibs = {{"arm64", "bin/macos/libnative.dylib"}};
    CHECK(nativeDllFor(both, "macos-arm64") == "bin/macos/libnative.dylib");
    CHECK(nativeDllFor(both, "macos-x64").empty());
    CHECK(nativeDllFor(both, "arm64") == "bin/arm64/native.dll");

    NativeContribution universal;
    universal.dylib = "libnative.dylib";
    CHECK(nativeDllFor(universal, "macos-x64") == "libnative.dylib");
    CHECK(nativeDllFor(universal, "x64").empty());
    CHECK(nativeDllField("macos") == "$.contributes.native.dylib");
    CHECK(nativeDllField("macos-arm64") == "$.contributes.native.dylib[\"arm64\"]");
#if defined(__APPLE__) && defined(__aarch64__)
    CHECK(hostArchitecture() == "macos-arm64");
#endif
}

TEST_CASE("a manifest's dylib is checked like its dll, and a manifest needs one of the two")
{
    const auto load = [](std::string_view native) {
        return loadManifest(std::string(R"({"schemaVersion":1,"id":"example.native","version":"1.0.0","name":"Native",)") +
                            R"("publisher":"Example","license":"MIT","engines":{"gamescrcpy":">=2.0.0"},)" +
                            R"("contributes":{"native":)" + std::string(native) + "}}");
    };
    const LoadedManifest both = load(R"({"dll":"native.dll","dylib":{"arm64":"lib/libnative.dylib"}})");
    CHECK_FALSE(hasCode(both.diagnostics, Code::NativeDllInvalid));
    CHECK_FALSE(std::ranges::any_of(both.diagnostics, [](const Diagnostic& diagnostic) {
        return diagnostic.code == Code::MissingField && diagnostic.path.starts_with("$.contributes.native");
    }));
    CHECK_FALSE(hasCode(both.diagnostics, Code::NativeNameInvalid));
    REQUIRE(both.manifest.contributes.native);
    CHECK(both.manifest.contributes.native->dylibs.at("arm64") == "lib/libnative.dylib");

    CHECK(hasCode(load(R"({"dylib":"lib/native.dll"})").diagnostics, Code::NativeDllInvalid));
    CHECK(hasCode(load(R"({"dll":"lib/native.dylib"})").diagnostics, Code::NativeDllInvalid));
    CHECK(hasCode(load(R"({"dylib":{"riscv":"lib/libnative.dylib"}})").diagnostics, Code::NativeArchitectureUnknown));
    const LoadedManifest none = load(R"({"nodes":[]})");
    CHECK(std::ranges::any_of(none.diagnostics, [](const Diagnostic& diagnostic) {
        return diagnostic.code == Code::MissingField && diagnostic.path == "$.contributes.native.dll";
    }));
}

TEST_CASE("something that is not a Mach-O library is not read, and says why, without reading past its end")
{
    CHECK_FALSE(looksLikeMachO(""));
    CHECK_FALSE(looksLikeMachO("MZ\x90\x00 not mach-o"));
    // A Java class file starts with the same bytes as a universal file but carries its version there.
    CHECK_FALSE(looksLikeMachO(bigEndian(0xCAFEBABE) + bigEndian(52)));

    const PeImports none = readMachOImports("hello");
    CHECK_FALSE(none.valid);
    CHECK_FALSE(none.problem.empty());

    std::string header(32, '\0');
    header[0] = '\xCF';
    header[1] = '\xFA';
    header[2] = '\xED';
    header[3] = '\xFE';
    header[16] = '\x01';
    header[20] = '\x40';
    CHECK_FALSE(readMachOImports(header).valid);
    CHECK_FALSE(readMachOExports(header).valid);
}

#if defined(GS_TEST_MACHO_CLEAN)

TEST_CASE("a real dylib: its processor, its gs_ext_main, and its imports with the library each comes from")
{
    const std::string probe = fileBytes(GS_TEST_MACHO_PROBE);
    REQUIRE(looksLikeMachO(probe));
    const PeExports exported = readMachOExports(probe);
    REQUIRE_MESSAGE(exported.valid, exported.problem);
    CHECK(std::ranges::find(exported.names, "gs_ext_main") != exported.names.end());
    CHECK(std::ranges::find(exported.names, "gs_probe_uses") != exported.names.end());
#if defined(__aarch64__)
    CHECK(exported.machine == kMachineMacArm64);
#endif

    const PeImports read = readMachOImports(probe);
    REQUIRE_MESSAGE(read.valid, read.problem);
    CHECK(imports(read, "libSystem.B.dylib", "socket"));
    CHECK(imports(read, "libSystem.B.dylib", "connect"));
    CHECK(imports(read, "libSystem.B.dylib", "fopen"));
    CHECK(imports(read, "libSystem.B.dylib", "posix_spawn"));
    CHECK(imports(read, "libSystem.B.dylib", "task_for_pid"));

    const std::vector<CapabilityUse> found = classifyImports(read.functions);
    CHECK(uses(found, Capability::Network));
    CHECK(uses(found, Capability::Files));
    CHECK(uses(found, Capability::Processes));
    CHECK(uses(found, Capability::Injection));
    CHECK_FALSE(uses(found, Capability::Input));

    const std::vector<CapabilityUse> clean = classifyImports(readMachOImports(fileBytes(GS_TEST_MACHO_CLEAN)).functions);
    CHECK(uses(clean, Capability::Files));
    CHECK_FALSE(uses(clean, Capability::Network));
    CHECK_FALSE(uses(clean, Capability::Injection));
}

// The same image with its symbol table emptied (LC_SYMTAB's count set to 0).
std::string withoutSymbols(std::string image)
{
    const auto le32 = [&image](std::size_t at) {
        std::uint32_t value = 0;
        for (std::size_t k = 4; k-- > 0;) {
            value = (value << 8) | static_cast<std::uint8_t>(image[at + k]);
        }
        return value;
    };
    std::size_t at = 32;
    for (std::uint32_t index = 0, count = le32(16); index < count; ++index) {
        if (le32(at) == 0x2) {
            image.replace(at + 12, 4, std::string(4, '\0'));
        }
        at += le32(at + 4);
    }
    return image;
}

TEST_CASE("imports are found from what dyld binds, in either format, even with the symbol table emptied")
{
    for (const char* path : {GS_TEST_MACHO_PROBE, GS_TEST_MACHO_PROBE_CLASSIC}) {
        CAPTURE(path);
        const std::string stripped = withoutSymbols(fileBytes(path));
        const PeImports read = readMachOImports(stripped);
        REQUIRE_MESSAGE(read.valid, read.problem);
        CHECK(imports(read, "libSystem.B.dylib", "socket"));
        CHECK(imports(read, "libSystem.B.dylib", "fopen"));
        CHECK(imports(read, "libSystem.B.dylib", "task_for_pid"));
        CHECK(uses(classifyImports(read.functions), Capability::Injection));
        const PeExports exported = readMachOExports(stripped);
        REQUIRE(exported.valid);
        CHECK(std::ranges::find(exported.names, "gs_ext_main") != exported.names.end());
    }
}

TEST_CASE("the same library inside a universal file is read from the slice of the wanted processor")
{
    const std::string thin = fileBytes(GS_TEST_MACHO_PROBE);
    const std::string fat = universal(thin);
    REQUIRE(looksLikeMachO(fat));
    const PeImports read = readMachOImports(fat, kMachineMacArm64);
    REQUIRE_MESSAGE(read.valid, read.problem);
    CHECK(read.machine == kMachineMacArm64);
    CHECK(imports(read, "libSystem.B.dylib", "task_for_pid"));
    const PeExports exported = readMachOExports(fat, kMachineMacArm64);
    REQUIRE(exported.valid);
    CHECK(std::ranges::find(exported.names, "gs_ext_main") != exported.names.end());
}

TEST_CASE("a damaged dylib is either read or refused with a reason, never read past its end")
{
    const std::string probe = fileBytes(GS_TEST_MACHO_PROBE);
    REQUIRE(probe.size() > 4096);
    for (std::size_t size = 0; size < probe.size(); size += std::max<std::size_t>(1, probe.size() / 97)) {
        const PeImports read = readMachOImports(probe.substr(0, size));
        CHECK((read.valid || !read.problem.empty()));
    }
    std::string flipped = probe;
    for (std::size_t at = 16; at < flipped.size(); at += 131) {
        flipped[at] = static_cast<char>(~flipped[at]);
        const PeImports read = readMachOImports(flipped);
        const PeExports exported = readMachOExports(flipped);
        CHECK((read.valid || !read.problem.empty()));
        CHECK((exported.valid || !exported.problem.empty()));
    }
}

TEST_CASE("a package's macOS dylib is checked like a Windows DLL: entry, processor, capabilities, injection")
{
    const std::string clean = fileBytes(GS_TEST_MACHO_CLEAN);
    const std::string probe = fileBytes(GS_TEST_MACHO_PROBE);

    const LibraryCheck listed = checkNativeLibraries(package({{"lib/clean.dylib", clean}}),
                                                     nativeManifest({{"arm64", "lib/clean.dylib"}}));
    CHECK(listed.diagnostics.empty());
    REQUIRE(listed.libraries.size() == 1);
    CHECK(listed.libraries.front().entry);

    const LibraryCheck universal =
        checkNativeLibraries(package({{"lib/clean.dylib", clean}}), nativeManifest({}, "lib/clean.dylib"));
    CHECK(universal.diagnostics.empty());

    const LibraryCheck asDll =
        checkNativeLibraries(package({{"lib/clean.dylib", clean}}), nativeManifest({}, {}, "lib/clean.dylib"));
    REQUIRE_FALSE(asDll.diagnostics.empty());
    CHECK(asDll.diagnostics.front().code == Code::NativeMachineMismatch);

    const NativeCheck safe = checkNative(package({{"lib/clean.dylib", clean}}),
                                         nativeManifest({{"arm64", "lib/clean.dylib"}}), {}, "macos-arm64");
    CHECK_FALSE(safe.refused());
    CHECK(uses(safe.capabilities, Capability::Files));
    CHECK(std::ranges::find(safe.uncovered, Capability::Files) != safe.uncovered.end());

    const NativeCheck injecting = checkNative(package({{"lib/probe.dylib", probe}}),
                                              nativeManifest({{"arm64", "lib/probe.dylib"}}), {}, "macos-arm64");
    CHECK(injecting.refusal == NativeRefusal::Injection);
    CHECK(injecting.detail.find("task_for_pid") != std::string::npos);

    const NativeCheck windowsOnly = checkNative(package({{"lib/clean.dylib", clean}}),
                                                nativeManifest({}, {}, "native.dll"), {}, "macos-arm64");
    CHECK(windowsOnly.refusal == NativeRefusal::Architecture);
}

#endif
