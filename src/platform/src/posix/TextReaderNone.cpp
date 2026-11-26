// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Systems without a text reader (Linux serves only as a test bed for the port): no language can be read.
#include "gs/platform/TextReader.h"

namespace gs::platform {

Expected<TextResult, TextProblem> readText(const std::uint8_t*, int, int, std::ptrdiff_t, std::string_view)
{
    return makeUnexpected(TextProblem::NoLanguage);
}

std::vector<std::string> textLanguages()
{
    return {};
}

TextLanguages::TextLanguages(Done done)
    : m_thread([done = std::move(done)] {
        if (done) {
            done({});
        }
    })
{
}

TextLanguages::~TextLanguages() = default;

}
