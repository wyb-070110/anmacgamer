# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
# gs_capture off Windows (the macOS port): the format, crop and pairing logic as on Windows; capture
# cards themselves (Media Foundation on Windows, AVFoundation on macOS) are not ported yet.
add_library(gs_capture STATIC
    include/gs/capture/AudioCapture.h
    include/gs/capture/AudioPairing.h
    include/gs/capture/CaptureDevices.h
    include/gs/capture/CaptureFormat.h
    include/gs/capture/CaptureSource.h
    include/gs/capture/CropDetector.h
    include/gs/capture/Encoders.h
    include/gs/capture/FrameBuilder.h
    include/gs/capture/MediaFoundationSource.h
    src/AudioPairing.cpp
    src/CaptureFormat.cpp
    src/CropDetector.cpp
    src/posix/AudioCapture.cpp
    src/posix/CaptureDevices.cpp
    src/posix/CaptureSource.cpp
    src/posix/Encoders.cpp
    src/posix/FrameBuilder.cpp
)
add_library(gs::capture ALIAS gs_capture)

target_include_directories(gs_capture PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(gs_capture PUBLIC gs::base gs::protocol gs::media PRIVATE gs::platform)
gs_apply_compiler_policy(gs_capture)
