# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
# gs_hardware off Windows: on macOS, IOKit enumerates and watches /dev/cu.* serial ports and termios opens
# them; on other POSIX systems, the fallback backend lists no devices.
add_library(gs_hardware STATIC
    include/gs/hardware/Backend.h
    include/gs/hardware/Devices.h
    include/gs/hardware/FakeBackend.h
    src/Devices.cpp
    src/FakeBackend.cpp
)
if(APPLE)
    target_sources(gs_hardware PRIVATE
        src/Worker.cpp
        src/Worker.h
        src/mac/SerialBackend.cpp
    )
    target_include_directories(gs_hardware PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(gs_hardware PRIVATE "-framework CoreFoundation" "-framework IOKit")
else()
    target_sources(gs_hardware PRIVATE src/posix/NoDevicesBackend.cpp)
endif()
add_library(gs::hardware ALIAS gs_hardware)

target_include_directories(gs_hardware PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(gs_hardware PUBLIC gs::base)
gs_apply_compiler_policy(gs_hardware)
