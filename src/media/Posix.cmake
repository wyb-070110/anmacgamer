# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
# gs_media off Windows (the macOS port): FFmpeg decodes with VideoToolbox when it can (Linux: on the
# processor), frames are read back by mapping them, sound goes out through SDL3. No HLSL shaders.
add_library(gs_media STATIC
    include/gs/media/AudioPlayer.h
    include/gs/media/AudioRegulator.h
    include/gs/media/ColorMatrix.h
    include/gs/media/FfmpegLog.h
    include/gs/media/FrameSnapshot.h
    include/gs/media/FrameUploader.h
    include/gs/media/LumaSampler.h
    include/gs/media/Thumbnailer.h
    include/gs/media/VideoDecoder.h
    src/AudioOutput.h
    src/AudioPlayer.cpp
    src/AudioRegulator.cpp
    src/ColorMatrix.cpp
    src/FfmpegLog.cpp
    src/FrameSnapshot.cpp
    src/LumaSampler.cpp
    src/PendingArrivals.cpp
    src/PendingArrivals.h
    src/PlaneReader.cpp
    src/PlaneReader.h
    src/Thumbnailer.cpp
    src/VideoDecoder.cpp
    src/posix/AudioOutput.cpp
    src/posix/FrameUploader.cpp
)
add_library(gs::media ALIAS gs_media)

target_include_directories(gs_media PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(gs_media
    PUBLIC gs::base gs::protocol FFmpeg::avutil
    PRIVATE FFmpeg::avcodec FFmpeg::swresample gs::platform
)
if(GS_HAVE_SDL3)
    target_link_libraries(gs_media PRIVATE SDL3::SDL3)
    target_compile_definitions(gs_media PRIVATE GS_HAVE_SDL3)
endif()
gs_apply_compiler_policy(gs_media)
