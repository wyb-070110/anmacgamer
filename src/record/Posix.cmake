# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
# gs_record off Windows: pictures through ImageIO on macOS (none on Linux); everything else as on Windows.
add_library(gs_record STATIC
    include/gs/record/MediaFile.h
    include/gs/record/Mp4Export.h
    include/gs/record/Operations.h
    include/gs/record/RecordedAudio.h
    include/gs/record/RecordedVision.h
    include/gs/record/Recorder.h
    include/gs/record/Recording.h
    src/MediaFile.cpp
    src/Mp4Export.cpp
    src/Operations.cpp
    src/Pictures.h
    src/RecordedAudio.cpp
    src/RecordedVision.cpp
    src/Recorder.cpp
    src/Recording.cpp
)
if(APPLE)
    target_sources(gs_record PRIVATE src/mac/Pictures.cpp)
    target_link_libraries(gs_record PRIVATE "-framework CoreFoundation" "-framework CoreGraphics" "-framework ImageIO"
                          "-framework ApplicationServices")
else()
    target_sources(gs_record PRIVATE src/posix/PicturesNone.cpp)
endif()
add_library(gs::record ALIAS gs_record)

target_include_directories(gs_record PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(gs_record
    PUBLIC gs::base gs::protocol gs::script gs::media
    PRIVATE gs::platform gs::vision FFmpeg::avformat FFmpeg::avcodec
)
gs_apply_compiler_policy(gs_record)
