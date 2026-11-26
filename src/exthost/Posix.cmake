# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
# gs_exthost off Windows (the macOS port). Channel, MediaRing, ImageSection, TapSection, InProcess, Server, Store,
# Wire and Hosted are the Windows files: their Win32 calls are mapped by src/posix/compat/windows.h onto POSIX in
# src/posix/Compat.cpp, and handles cross between the app and gs-exthost over a Unix socket. Only launching
# gs-exthost (src/posix/Launch.inc, Spawn.cpp) and the container of a restricted extension (src/posix/Container.cpp,
# the macOS sandbox instead of an AppContainer) are written anew.
add_library(gs_exthost STATIC
    include/gs/exthost/Hosted.h
    include/gs/exthost/InProcess.h
    include/gs/exthost/Media.h
    include/gs/exthost/Module.h
    include/gs/exthost/Server.h
    src/Channel.cpp
    src/Channel.h
    src/Container.h
    src/Hosted.cpp
    src/ImageSection.cpp
    src/ImageSection.h
    src/InProcess.cpp
    src/MediaRing.cpp
    src/MediaRing.h
    src/Module.cpp
    src/Server.cpp
    src/Store.cpp
    src/Store.h
    src/TapSection.cpp
    src/TapSection.h
    src/Wire.cpp
    src/Wire.h
    src/posix/Bridge.h
    src/posix/Compat.cpp
    src/posix/Container.cpp
    src/posix/Launch.inc
    src/posix/Sandbox.h
    src/posix/Spawn.cpp
    src/posix/compat/windows.h
)
add_library(gs::exthost ALIAS gs_exthost)

target_include_directories(gs_exthost PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${PROJECT_SOURCE_DIR}/sdk/include")
target_include_directories(gs_exthost BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/posix/compat")
target_link_libraries(gs_exthost PUBLIC gs::base PRIVATE gs::nlohmann_json gs::platform gs::ext ${CMAKE_DL_LIBS})
gs_apply_compiler_policy(gs_exthost)

add_executable(gs_exthost_program posix/main.cpp)
set_target_properties(gs_exthost_program PROPERTIES OUTPUT_NAME gs-exthost)
target_link_libraries(gs_exthost_program PRIVATE gs::exthost)
gs_apply_compiler_policy(gs_exthost_program)

add_subdirectory("${PROJECT_SOURCE_DIR}/examples/extensions/pc-clock" "${PROJECT_BINARY_DIR}/examples/pc-clock")
