// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
#include <gs/hardware/Backend.h>

#include <doctest/doctest.h>

#include <optional>

using namespace gs::hardware;

TEST_CASE("off Windows the system backend lists no devices and opens none, as if each were unplugged")
{
    const std::unique_ptr<Backend> backend = makeWindowsBackend();
    REQUIRE(backend != nullptr);
    CHECK(backend->devices().empty());

    bool changed = false;
    backend->watch([&changed] { changed = true; });
    backend->scan(true);
    backend->scan(false);
    CHECK(backend->devices().empty());
    CHECK_FALSE(changed);

    DeviceInfo board;
    board.id = "serial:/dev/cu.usbserial-0001";
    board.kind = Kind::Serial;
    std::optional<OpenResult> opened;
    backend->open(OpenRequest{board, {}, nullptr}, [&opened](OpenResult result) { opened = std::move(result); });
    REQUIRE(opened.has_value());
    CHECK(opened->device == nullptr);
    CHECK(opened->missing);
    CHECK_FALSE(opened->problem.empty());
}
