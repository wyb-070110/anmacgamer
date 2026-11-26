// SPDX-License-Identifier: GPL-3.0-or-later
#include "CompanionCipher.h"

#include <gs/platform/Crypto.h>
#include <gs/session/CompanionMirror.h>

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <span>
#include <string>
#include <vector>

using namespace gs;
using namespace gs::app;
using nlohmann::json;

namespace {

std::vector<std::uint8_t> fromHex(const std::string& text)
{
    std::vector<std::uint8_t> bytes;
    for (std::size_t i = 0; i + 1 < text.size(); i += 2) {
        bytes.push_back(static_cast<std::uint8_t>(std::stoi(text.substr(i, 2), nullptr, 16)));
    }
    return bytes;
}

template <std::size_t N>
std::array<std::uint8_t, N> fixed(const std::string& text)
{
    const std::vector<std::uint8_t> bytes = fromHex(text);
    REQUIRE(bytes.size() == N);
    std::array<std::uint8_t, N> out{};
    std::ranges::copy(bytes, out.begin());
    return out;
}

json mirrorVectors()
{
    std::ifstream file(std::filesystem::path(GS_PROTOCOL_VECTOR_DIR) / "companion" / "mirror.json", std::ios::binary);
    REQUIRE(file.good());
    const json document = json::parse(file);
    REQUIRE(document.at("format") == "gamescrcpy.companion-mirror-vectors");
    return document.at("vectors").at(0);
}

LanDirection directionOf(const json& record)
{
    return record.at("direction") == "toCompanion" ? LanDirection::ToCompanion : LanDirection::ToComputer;
}

}

TEST_CASE("SPEC 11.10: the mirror handshake, session key, session token and sealed records match the shared vectors")
{
    const json vectors = mirrorVectors();
    const auto key = fixed<kLanKeySize>(vectors.at("key"));
    const auto phone = fixed<kLanIdSize>(vectors.at("phone"));
    const auto pc = fixed<kLanIdSize>(vectors.at("pc"));
    CHECK(vectors.at("context") == std::string(kMirrorContext));
    CHECK(vectors.at("tokenContext") == std::string(kMirrorTokenContext));

    const std::vector<std::uint8_t> hello = fromHex(vectors.at("hello"));
    const std::vector<std::uint8_t> reply = fromHex(vectors.at("reply"));
    CHECK(isMirrorHello(hello));
    CHECK_FALSE(isMirrorHello(encodeLanHello({.phone = phone})));
    const auto decodedHello = decodeLanHello(hello);
    REQUIRE(decodedHello.has_value());
    CHECK(decodedHello->token.empty());
    CHECK(decodedHello->phone == phone);
    CHECK(lanHelloSize(hello).value() == hello.size());
    const auto decodedReply = decodeLanReply(reply);
    REQUIRE(decodedReply.has_value());
    CHECK(decodedReply->pc == pc);
    CHECK(encodeLanReply(*decodedReply, true) == reply);

    const LanKey session = mirrorSessionKey(key, hello, reply);
    CHECK(platform::toHex(session) == vectors.at("sessionKey").get<std::string>());
    CHECK(session != lanSessionKey(key, hello, reply));
    CHECK(platform::toHex(mirrorToken(session)) == vectors.at("token").get<std::string>());

    std::map<LanDirection, LanSealer> sealers;
    std::map<LanDirection, LanOpener> openers;
    for (const LanDirection direction : {LanDirection::ToCompanion, LanDirection::ToComputer}) {
        sealers.emplace(direction, LanSealer(session, direction));
        openers.emplace(direction, LanOpener(session, direction, 0, true));
    }
    for (const json& record : vectors.at("records")) {
        INFO(record.at("what").get<std::string>());
        const LanDirection direction = directionOf(record);
        const std::vector<std::uint8_t> mux = fromHex(record.at("mux"));
        const std::vector<std::uint8_t> sealed = fromHex(record.at("hex"));
        CHECK(platform::toHex(sealers.at(direction).seal(mux)) == record.at("hex").get<std::string>());
        LanPrefix prefix{};
        std::copy_n(sealed.begin(), 4, prefix.begin());
        CHECK(LanOpener::mirrorRecordLength(prefix).value() == sealed.size() - 4);
        const auto opened = openers.at(direction).open(prefix, std::span(sealed).subspan(4));
        REQUIRE(opened.has_value());
        CHECK(*opened == mux);
        const auto from =
            direction == LanDirection::ToCompanion ? session::MirrorFrom::Computer : session::MirrorFrom::Companion;
        CHECK(session::decodeMirrorRecord(*opened, from).has_value());
    }
}

TEST_CASE("SPEC 11.10: mirror records of a wrong length or under the wrong key do not open, and refusals carry the "
          "mirror magic")
{
    const json vectors = mirrorVectors();
    const auto key = fixed<kLanKeySize>(vectors.at("key"));
    const LanKey session = mirrorSessionKey(key, fromHex(vectors.at("hello")), fromHex(vectors.at("reply")));
    for (const json& bad : vectors.at("malformedRecords")) {
        INFO(bad.at("name").get<std::string>());
        const std::vector<std::uint8_t> sealed = fromHex(bad.at("hex"));
        LanPrefix prefix{};
        std::copy_n(sealed.begin(), 4, prefix.begin());
        LanOpener opener(session, directionOf(bad), bad.at("counter").get<std::uint64_t>(), true);
        const auto opened = opener.open(prefix, std::span(sealed).subspan(4));
        REQUIRE_FALSE(opened.has_value());
        CHECK(lanErrorName(opened.error()) == bad.at("error").get<std::string>());
    }
    for (const json& refused : vectors.at("refused")) {
        INFO(refused.at("name").get<std::string>());
        CHECK(isMirrorHello(fromHex(refused.at("hello"))));
        CHECK(encodeLanReply({.status = LanStatus::Unknown}, true) == fromHex(refused.at("reply")));
    }
}
