// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
// Systems without an image reader (Linux serves only as a test bed for the port): pictures are unreadable.
#include "Pictures.h"

namespace gs::record {

struct PictureReader::State {};

PictureReader::PictureReader() : m_state(std::make_unique<State>()) {}

PictureReader::~PictureReader() = default;

PictureProblem PictureReader::read(const std::filesystem::path& file, Picture& picture)
{
    picture = {};
    std::error_code error;
    return std::filesystem::is_regular_file(file, error) ? PictureProblem::Unreadable : PictureProblem::Missing;
}

Picture PictureReader::scaled(const Picture& picture, int, int)
{
    return picture;
}

}
