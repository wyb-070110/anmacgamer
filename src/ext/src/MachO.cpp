// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Mach-O libraries for the macOS build of a native extension: which processor a dylib is built for, what it
// exports, and what it imports from which library. Runs on every system, so gs validate on Windows checks the
// macOS libraries of a package too.
//
// Imports are taken from what dyld binds (chained fixups, or the bind opcodes of LC_DYLD_INFO) and from the
// undefined symbols of the symbol table: stripping the symbol table cannot hide an import from the check.
// Exports come from the export trie, or from the symbol table when there is none.
#include "gs/ext/NativeCheck.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gs::ext {

namespace {

constexpr std::uint32_t kMagic64 = 0xFEEDFACF;
constexpr std::uint32_t kMagic32 = 0xFEEDFACE;
constexpr std::uint32_t kFatMagic = 0xCAFEBABE;
constexpr std::uint32_t kFatMagic64 = 0xCAFEBABF;
constexpr std::uint32_t kCpuArm64 = 0x0100000C;
constexpr std::uint32_t kCpuX64 = 0x01000007;
constexpr std::uint32_t kFileExecute = 2;
constexpr std::uint32_t kFileDylib = 6;
constexpr std::uint32_t kFileBundle = 8;

constexpr std::uint32_t kLcSymtab = 0x2;
constexpr std::uint32_t kLcLoadDylib = 0xC;
constexpr std::uint32_t kLcLoadWeakDylib = 0x80000018;
constexpr std::uint32_t kLcReexportDylib = 0x8000001F;
constexpr std::uint32_t kLcLazyLoadDylib = 0x20;
constexpr std::uint32_t kLcLoadUpwardDylib = 0x80000023;
constexpr std::uint32_t kLcDyldInfo = 0x22;
constexpr std::uint32_t kLcDyldInfoOnly = 0x80000022;
constexpr std::uint32_t kLcExportsTrie = 0x80000033;
constexpr std::uint32_t kLcChainedFixups = 0x80000034;

constexpr std::uint32_t kMostCommands = 4096;
constexpr std::uint32_t kMostFatSlices = 16;
constexpr std::uint32_t kMostSymbols = 1'000'000;
constexpr std::size_t kMostImports = 200'000;
constexpr std::size_t kMostExports = 65'536;
constexpr std::size_t kMostTrieNodes = 1'000'000;
constexpr std::size_t kMostNameBytes = 4096;

class Bytes
{
public:
    explicit Bytes(std::string_view data) noexcept : m_data(data) {}

    [[nodiscard]] std::size_t size() const noexcept { return m_data.size(); }

    [[nodiscard]] bool fits(std::uint64_t offset, std::uint64_t length) const noexcept
    {
        return offset <= m_data.size() && length <= m_data.size() - offset;
    }

    [[nodiscard]] std::uint32_t le32(std::uint64_t offset) const noexcept { return value<std::uint32_t>(offset, false); }
    [[nodiscard]] std::uint64_t le64(std::uint64_t offset) const noexcept { return value<std::uint64_t>(offset, false); }
    [[nodiscard]] std::uint16_t le16(std::uint64_t offset) const noexcept { return value<std::uint16_t>(offset, false); }
    [[nodiscard]] std::uint32_t be32(std::uint64_t offset) const noexcept { return value<std::uint32_t>(offset, true); }
    [[nodiscard]] std::uint64_t be64(std::uint64_t offset) const noexcept { return value<std::uint64_t>(offset, true); }
    [[nodiscard]] std::uint8_t byte(std::uint64_t offset) const noexcept
    {
        return static_cast<std::uint8_t>(m_data[static_cast<std::size_t>(offset)]);
    }

    // A NUL-terminated string starting at offset and ending before limit.
    [[nodiscard]] std::optional<std::string> text(std::uint64_t offset, std::uint64_t limit) const
    {
        limit = std::min<std::uint64_t>(limit, m_data.size());
        if (offset >= limit) {
            return std::nullopt;
        }
        const std::string_view rest = m_data.substr(static_cast<std::size_t>(offset),
                                                    static_cast<std::size_t>(limit - offset));
        const std::size_t end = rest.find('\0');
        if (end == std::string_view::npos || end > kMostNameBytes) {
            return std::nullopt;
        }
        return std::string(rest.substr(0, end));
    }

    [[nodiscard]] Bytes slice(std::uint64_t offset, std::uint64_t length) const noexcept
    {
        return Bytes(m_data.substr(static_cast<std::size_t>(offset), static_cast<std::size_t>(length)));
    }

private:
    template <typename T>
    [[nodiscard]] T value(std::uint64_t offset, bool big) const noexcept
    {
        T out = 0;
        for (std::size_t k = 0; k < sizeof(T); ++k) {
            const auto b = static_cast<T>(static_cast<std::uint8_t>(m_data[static_cast<std::size_t>(offset) + k]));
            out |= big ? static_cast<T>(b << (8 * (sizeof(T) - 1 - k))) : static_cast<T>(b << (8 * k));
        }
        return out;
    }

    std::string_view m_data;
};

std::optional<std::uint64_t> uleb(const Bytes& bytes, std::uint64_t& at, std::uint64_t end)
{
    std::uint64_t result = 0;
    for (int shift = 0; shift < 64; shift += 7) {
        if (at >= end) {
            return std::nullopt;
        }
        const std::uint8_t b = bytes.byte(at++);
        result |= static_cast<std::uint64_t>(b & 0x7F) << shift;
        if ((b & 0x80) == 0) {
            return result;
        }
    }
    return std::nullopt;
}

bool skipLeb(const Bytes& bytes, std::uint64_t& at, std::uint64_t end)
{
    for (int k = 0; k < 10; ++k) {
        if (at >= end) {
            return false;
        }
        if ((bytes.byte(at++) & 0x80) == 0) {
            return true;
        }
    }
    return false;
}

std::uint16_t machineOf(std::uint32_t cpu) noexcept
{
    if (cpu == kCpuArm64) {
        return kMachineMacArm64;
    }
    if (cpu == kCpuX64) {
        return kMachineMacX64;
    }
    return 0;
}

// The one image in a thin or universal file: the slice for wanted when there is one, else the first 64-bit one.
struct Thin {
    Bytes image{std::string_view()};
    std::uint32_t cpu = 0;
};

std::optional<Thin> pickImage(std::string_view data, std::uint16_t wanted, std::string& problem)
{
    const Bytes bytes(data);
    if (!bytes.fits(0, 8)) {
        problem = "the file is too small to be a Mach-O library";
        return std::nullopt;
    }
    const std::uint32_t magic = bytes.le32(0);
    if (magic == kMagic64) {
        return Thin{bytes, bytes.le32(4)};
    }
    if (magic == kMagic32) {
        problem = "32-bit Mach-O libraries are not supported";
        return std::nullopt;
    }
    const std::uint32_t fat = bytes.be32(0);
    if (fat != kFatMagic && fat != kFatMagic64) {
        problem = "the file is not a Mach-O library";
        return std::nullopt;
    }
    const std::uint32_t count = bytes.be32(4);
    const std::uint64_t entry = fat == kFatMagic64 ? 32 : 20;
    if (count == 0 || count > kMostFatSlices || !bytes.fits(8, count * entry)) {
        problem = "the universal header is damaged";
        return std::nullopt;
    }
    std::optional<Thin> first;
    for (std::uint32_t index = 0; index < count; ++index) {
        const std::uint64_t at = 8 + index * entry;
        const std::uint32_t cpu = bytes.be32(at);
        const std::uint64_t offset = fat == kFatMagic64 ? bytes.be64(at + 8) : bytes.be32(at + 8);
        const std::uint64_t size = fat == kFatMagic64 ? bytes.be64(at + 16) : bytes.be32(at + 12);
        if (!bytes.fits(offset, size) || size < 32) {
            problem = "a slice of the universal file lies outside it";
            return std::nullopt;
        }
        Thin thin{bytes.slice(offset, size), cpu};
        if (thin.image.le32(0) != kMagic64) {
            continue;
        }
        if (wanted != 0 && machineOf(cpu) == wanted) {
            return thin;
        }
        if (!first) {
            first = thin;
        }
    }
    if (!first) {
        problem = "the universal file has no 64-bit library";
    }
    return first;
}

struct Image {
    std::uint32_t cpu = 0;
    std::uint32_t fileType = 0;
    std::vector<std::string> libraries;
    std::optional<std::pair<std::uint32_t, std::uint32_t>> symtab;
    std::optional<std::pair<std::uint32_t, std::uint32_t>> strings;
    std::optional<std::pair<std::uint32_t, std::uint32_t>> exportsTrie;
    std::optional<std::pair<std::uint32_t, std::uint32_t>> chainedFixups;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> bindStreams;
};

std::string libraryName(const std::string& installName)
{
    const std::size_t slash = installName.rfind('/');
    return slash == std::string::npos ? installName : installName.substr(slash + 1);
}

std::string readCommands(const Bytes& bytes, std::uint32_t cpu, Image& image)
{
    if (!bytes.fits(0, 32)) {
        return "the Mach-O header is damaged";
    }
    image.cpu = cpu;
    image.fileType = bytes.le32(12);
    const std::uint32_t count = bytes.le32(16);
    const std::uint32_t total = bytes.le32(20);
    if (count > kMostCommands || !bytes.fits(32, total)) {
        return "the Mach-O load commands are damaged";
    }
    const auto range = [&bytes](std::uint32_t offset, std::uint32_t size) {
        return bytes.fits(offset, size) ? std::optional(std::pair(offset, size)) : std::nullopt;
    };
    std::uint64_t at = 32;
    const std::uint64_t end = 32 + static_cast<std::uint64_t>(total);
    for (std::uint32_t index = 0; index < count; ++index) {
        if (at + 8 > end) {
            return "the Mach-O load commands are damaged";
        }
        const std::uint32_t command = bytes.le32(at);
        const std::uint32_t size = bytes.le32(at + 4);
        if (size < 8 || at + size > end) {
            return "the Mach-O load commands are damaged";
        }
        switch (command) {
        case kLcLoadDylib:
        case kLcLoadWeakDylib:
        case kLcReexportDylib:
        case kLcLazyLoadDylib:
        case kLcLoadUpwardDylib: {
            if (size < 24) {
                return "a library reference is damaged";
            }
            const std::optional<std::string> name = bytes.text(at + bytes.le32(at + 8), at + size);
            if (!name) {
                return "a library reference is damaged";
            }
            image.libraries.push_back(libraryName(*name));
            break;
        }
        case kLcSymtab:
            if (size < 24) {
                return "the symbol table command is damaged";
            }
            image.symtab = std::pair(bytes.le32(at + 8), bytes.le32(at + 12));
            image.strings = range(bytes.le32(at + 16), bytes.le32(at + 20));
            if (!image.strings) {
                return "the string table lies outside the file";
            }
            break;
        case kLcDyldInfo:
        case kLcDyldInfoOnly:
            if (size < 48) {
                return "the dyld information command is damaged";
            }
            for (const std::uint64_t field : {16u, 24u, 32u}) {
                if (const auto stream = range(bytes.le32(at + field), bytes.le32(at + field + 4))) {
                    image.bindStreams.push_back(*stream);
                } else {
                    return "the bind information lies outside the file";
                }
            }
            if (bytes.le32(at + 44) != 0) {
                image.exportsTrie = range(bytes.le32(at + 40), bytes.le32(at + 44));
                if (!image.exportsTrie) {
                    return "the export information lies outside the file";
                }
            }
            break;
        case kLcExportsTrie:
            if (size < 16) {
                return "the export trie command is damaged";
            }
            image.exportsTrie = range(bytes.le32(at + 8), bytes.le32(at + 12));
            if (!image.exportsTrie) {
                return "the export trie lies outside the file";
            }
            break;
        case kLcChainedFixups:
            if (size < 16) {
                return "the chained fixups command is damaged";
            }
            image.chainedFixups = range(bytes.le32(at + 8), bytes.le32(at + 12));
            if (!image.chainedFixups) {
                return "the chained fixups lie outside the file";
            }
            break;
        default:
            break;
        }
        at += size;
    }
    return {};
}

// "_name" in C becomes "name"; an Objective-C class reference stays as it is ("_OBJC_CLASS_$_NSTask" -> "OBJC_CLASS_$_NSTask").
std::string symbolName(std::string_view raw)
{
    if (raw.starts_with('_')) {
        raw.remove_prefix(1);
    }
    return std::string(raw);
}

class Imports
{
public:
    explicit Imports(const Image& image) : m_image(image) {}

    // ordinal: 0 this image, 1..n its libraries, negative values the main program or a flat lookup.
    bool add(std::int64_t ordinal, std::string_view raw)
    {
        if (raw.empty()) {
            return true;
        }
        std::string module;
        if (ordinal >= 1 && static_cast<std::uint64_t>(ordinal) <= m_image.libraries.size()) {
            module = m_image.libraries[static_cast<std::size_t>(ordinal - 1)];
        } else if (ordinal == 0) {
            return true;
        } else {
            module = ordinal == -1 ? "(main program)" : "(any library)";
        }
        if (m_seen.emplace(module, std::string(raw)).second) {
            if (m_seen.size() > kMostImports) {
                return false;
            }
            m_functions.push_back({.module = std::move(module), .name = symbolName(raw), .macho = true});
        }
        return true;
    }

    [[nodiscard]] std::vector<ImportedFunction> take() { return std::move(m_functions); }

private:
    const Image& m_image;
    std::set<std::pair<std::string, std::string>> m_seen;
    std::vector<ImportedFunction> m_functions;
};

std::string readSymbolImports(const Bytes& bytes, const Image& image, Imports& imports)
{
    if (!image.symtab || !image.strings) {
        return {};
    }
    const auto [offset, count] = *image.symtab;
    if (count > kMostSymbols || !bytes.fits(offset, static_cast<std::uint64_t>(count) * 16)) {
        return "the symbol table lies outside the file";
    }
    const auto [stringOffset, stringSize] = *image.strings;
    for (std::uint32_t index = 0; index < count; ++index) {
        const std::uint64_t at = offset + static_cast<std::uint64_t>(index) * 16;
        const std::uint8_t type = bytes.byte(at + 4);
        const std::uint16_t desc = bytes.le16(at + 6);
        if ((type & 0xE0) != 0 || (type & 0x0E) != 0 || (type & 0x01) == 0) {
            continue;
        }
        const std::uint32_t name = bytes.le32(at);
        if (name >= stringSize) {
            return "a symbol name lies outside the string table";
        }
        const std::optional<std::string> text =
            bytes.text(static_cast<std::uint64_t>(stringOffset) + name, static_cast<std::uint64_t>(stringOffset) + stringSize);
        if (!text) {
            return "a symbol name is damaged";
        }
        const std::uint8_t ordinal = static_cast<std::uint8_t>(desc >> 8);
        // 0 in a flat-namespace image (or an undefined symbol said to be in this one) is looked up anywhere.
        const std::int64_t library = ordinal == 0xFE ? -1 : (ordinal == 0xFF || ordinal == 0) ? -2 : ordinal;
        if (!imports.add(library, *text)) {
            return "the library imports more functions than a package can need";
        }
    }
    return {};
}

std::string readBindImports(const Bytes& bytes, const Image& image, Imports& imports)
{
    for (const auto& [offset, size] : image.bindStreams) {
        std::uint64_t at = offset;
        const std::uint64_t end = static_cast<std::uint64_t>(offset) + size;
        std::int64_t ordinal = 0;
        while (at < end) {
            const std::uint8_t op = bytes.byte(at++);
            const std::uint8_t immediate = op & 0x0F;
            switch (op & 0xF0) {
            case 0x00:
                break;
            case 0x10:
                ordinal = immediate;
                break;
            case 0x20:
                if (const auto value = uleb(bytes, at, end)) {
                    ordinal = static_cast<std::int64_t>(*value);
                } else {
                    return "the bind information is damaged";
                }
                break;
            case 0x30:
                ordinal = immediate == 0 ? 0 : -static_cast<std::int64_t>(16 - immediate);
                break;
            case 0x40: {
                const std::optional<std::string> name = bytes.text(at, end);
                if (!name) {
                    return "the bind information is damaged";
                }
                at += name->size() + 1;
                if (!imports.add(ordinal, *name)) {
                    return "the library imports more functions than a package can need";
                }
                break;
            }
            case 0x50:
            case 0x90:
            case 0xB0:
                break;
            case 0x60:
            case 0x70:
            case 0x80:
            case 0xA0:
                if (!skipLeb(bytes, at, end)) {
                    return "the bind information is damaged";
                }
                break;
            case 0xC0:
                if (!skipLeb(bytes, at, end) || !skipLeb(bytes, at, end)) {
                    return "the bind information is damaged";
                }
                break;
            case 0xD0:
                if (immediate == 0 && !skipLeb(bytes, at, end)) {
                    return "the bind information is damaged";
                }
                break;
            default:
                return "the bind information has an unknown instruction";
            }
        }
    }
    return {};
}

std::string readChainedImports(const Bytes& bytes, const Image& image, Imports& imports)
{
    if (!image.chainedFixups) {
        return {};
    }
    const auto [offset, size] = *image.chainedFixups;
    const Bytes fixups = bytes.slice(offset, size);
    if (!fixups.fits(0, 28)) {
        return "the chained fixups are damaged";
    }
    const std::uint32_t importsOffset = fixups.le32(8);
    const std::uint32_t symbolsOffset = fixups.le32(12);
    const std::uint32_t count = fixups.le32(16);
    const std::uint32_t format = fixups.le32(20);
    const std::uint32_t symbolsFormat = fixups.le32(24);
    const std::uint64_t entry = format == 1 ? 4 : format == 2 ? 8 : format == 3 ? 16 : 0;
    if (entry == 0 || symbolsFormat != 0) {
        return "the chained fixups use a format GameScrcpy cannot read";
    }
    if (count > kMostImports || !fixups.fits(importsOffset, static_cast<std::uint64_t>(count) * entry) ||
        symbolsOffset > fixups.size()) {
        return "the chained fixups are damaged";
    }
    for (std::uint32_t index = 0; index < count; ++index) {
        const std::uint64_t at = importsOffset + static_cast<std::uint64_t>(index) * entry;
        std::int64_t ordinal = 0;
        std::uint64_t name = 0;
        if (format == 3) {
            const std::uint64_t word = fixups.le64(at);
            const auto raw = static_cast<std::uint16_t>(word & 0xFFFF);
            ordinal = raw >= 0xFFF0 ? static_cast<std::int16_t>(raw) : raw;
            name = word >> 32;
        } else {
            const std::uint32_t word = fixups.le32(at);
            const auto raw = static_cast<std::uint8_t>(word & 0xFF);
            ordinal = raw >= 0xF0 ? static_cast<std::int8_t>(raw) : raw;
            name = word >> 9;
        }
        const std::optional<std::string> text = fixups.text(symbolsOffset + name, fixups.size());
        if (!text) {
            return "a chained import name is damaged";
        }
        if (!imports.add(ordinal, *text)) {
            return "the library imports more functions than a package can need";
        }
    }
    return {};
}

std::string readTrie(const Bytes& bytes, std::pair<std::uint32_t, std::uint32_t> trie, std::vector<std::string>& names)
{
    const auto [offset, size] = trie;
    const std::uint64_t end = static_cast<std::uint64_t>(offset) + size;
    std::vector<std::pair<std::uint64_t, std::string>> pending{{offset, std::string()}};
    std::set<std::uint64_t> visited;
    while (!pending.empty()) {
        auto [node, prefix] = std::move(pending.back());
        pending.pop_back();
        if (!visited.insert(node).second || visited.size() > kMostTrieNodes || node >= end) {
            return "the export trie is damaged";
        }
        std::uint64_t at = node;
        const std::optional<std::uint64_t> terminal = uleb(bytes, at, end);
        if (!terminal || *terminal > end - at) {
            return "the export trie is damaged";
        }
        if (*terminal != 0) {
            names.push_back(symbolName(prefix));
            if (names.size() > kMostExports) {
                return "the library exports too many names";
            }
        }
        at += *terminal;
        if (at >= end) {
            return "the export trie is damaged";
        }
        const std::uint8_t children = bytes.byte(at++);
        for (std::uint8_t child = 0; child < children; ++child) {
            const std::optional<std::string> edge = bytes.text(at, end);
            if (!edge) {
                return "the export trie is damaged";
            }
            at += edge->size() + 1;
            const std::optional<std::uint64_t> next = uleb(bytes, at, end);
            if (!next || prefix.size() + edge->size() > kMostNameBytes) {
                return "the export trie is damaged";
            }
            pending.emplace_back(static_cast<std::uint64_t>(offset) + *next, prefix + *edge);
        }
    }
    return {};
}

std::string readSymbolExports(const Bytes& bytes, const Image& image, std::vector<std::string>& names)
{
    if (!image.symtab || !image.strings) {
        return {};
    }
    const auto [offset, count] = *image.symtab;
    if (count > kMostSymbols || !bytes.fits(offset, static_cast<std::uint64_t>(count) * 16)) {
        return "the symbol table lies outside the file";
    }
    const auto [stringOffset, stringSize] = *image.strings;
    for (std::uint32_t index = 0; index < count; ++index) {
        const std::uint64_t at = offset + static_cast<std::uint64_t>(index) * 16;
        const std::uint8_t type = bytes.byte(at + 4);
        if ((type & 0xE0) != 0 || (type & 0x0E) != 0x0E || (type & 0x01) == 0) {
            continue;
        }
        const std::uint32_t name = bytes.le32(at);
        const std::optional<std::string> text =
            name < stringSize ? bytes.text(static_cast<std::uint64_t>(stringOffset) + name,
                                           static_cast<std::uint64_t>(stringOffset) + stringSize)
                              : std::nullopt;
        if (!text) {
            return "a symbol name is damaged";
        }
        names.push_back(symbolName(*text));
        if (names.size() > kMostExports) {
            return "the library exports too many names";
        }
    }
    return {};
}

std::string libraryProblem(const Image& image)
{
    if (image.fileType != kFileDylib && image.fileType != kFileBundle) {
        return image.fileType == kFileExecute ? "the file is a program, not a library" : "the file is not a library";
    }
    return {};
}

}

bool looksLikeMachO(std::string_view bytes) noexcept
{
    const Bytes data(bytes);
    if (!data.fits(0, 8)) {
        return false;
    }
    const std::uint32_t magic = data.le32(0);
    const std::uint32_t fat = data.be32(0);
    // 0xCAFEBABE is also a Java class file; those carry a version (>= 45) where a universal file has its slice count.
    return magic == kMagic64 || magic == kMagic32 || fat == kFatMagic64 ||
           (fat == kFatMagic && data.be32(4) > 0 && data.be32(4) <= kMostFatSlices);
}

PeImports readMachOImports(std::string_view data, std::uint16_t wanted)
{
    PeImports result;
    std::string problem;
    const std::optional<Thin> thin = pickImage(data, wanted, problem);
    Image image;
    if (thin) {
        problem = readCommands(thin->image, thin->cpu, image);
    }
    if (problem.empty()) {
        problem = libraryProblem(image);
    }
    Imports imports(image);
    if (problem.empty()) {
        problem = readChainedImports(thin->image, image, imports);
    }
    if (problem.empty()) {
        problem = readBindImports(thin->image, image, imports);
    }
    if (problem.empty()) {
        problem = readSymbolImports(thin->image, image, imports);
    }
    result.valid = problem.empty();
    result.wide = true;
    result.machine = thin ? machineOf(thin->cpu) : 0;
    if (result.valid) {
        result.functions = imports.take();
    } else {
        result.problem = std::move(problem);
    }
    return result;
}

PeExports readMachOExports(std::string_view data, std::uint16_t wanted)
{
    PeExports result;
    std::string problem;
    const std::optional<Thin> thin = pickImage(data, wanted, problem);
    Image image;
    if (thin) {
        problem = readCommands(thin->image, thin->cpu, image);
    }
    if (problem.empty()) {
        problem = libraryProblem(image);
    }
    if (problem.empty()) {
        problem = image.exportsTrie ? readTrie(thin->image, *image.exportsTrie, result.names)
                                    : readSymbolExports(thin->image, image, result.names);
    }
    result.valid = problem.empty();
    result.machine = thin ? machineOf(thin->cpu) : 0;
    if (!result.valid) {
        result.names.clear();
        result.problem = std::move(problem);
    }
    return result;
}

}
