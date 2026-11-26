# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
# gs_platform off Windows (macOS port): the POSIX backend under src/posix. Only what the core modules
# below the interface need so far; the rest of the headers stay unimplemented until a module uses them.
if(APPLE AND NOT OPENSSL_ROOT_DIR AND NOT DEFINED ENV{OPENSSL_ROOT_DIR})
    foreach(gs_openssl_prefix IN ITEMS /opt/homebrew/opt/openssl@3 /usr/local/opt/openssl@3)
        if(EXISTS "${gs_openssl_prefix}/include/openssl/evp.h")
            set(OPENSSL_ROOT_DIR "${gs_openssl_prefix}")
            break()
        endif()
    endforeach()
endif()
find_package(OpenSSL 3 REQUIRED COMPONENTS Crypto)
find_package(Threads REQUIRED)

add_library(gs_platform STATIC
    include/gs/platform/Accessibility.h
    include/gs/platform/AtomicFile.h
    include/gs/platform/CrashHandler.h
    include/gs/platform/Credentials.h
    include/gs/platform/Cursor.h
    include/gs/platform/Foreground.h
    include/gs/platform/InputCapture.h
    include/gs/platform/KeyboardLayout.h
    include/gs/platform/Speech.h
    include/gs/platform/StreamingPower.h
    include/gs/platform/TrayNotice.h
    include/gs/platform/Usb.h
    include/gs/platform/Crypto.h
    include/gs/platform/QuitSignal.h
    include/gs/platform/RunningMarker.h
    include/gs/platform/TcpStream.h
    include/gs/platform/TextReader.h
    include/gs/platform/Thread.h
    include/gs/platform/UdpSocket.h
    src/posix/AtomicFile.cpp
    src/posix/CrashHandler.cpp
    src/posix/Crypto.cpp
    src/posix/QuitSignal.cpp
    src/posix/RunningMarker.cpp
    src/posix/TcpStream.cpp
    src/posix/TextJoin.cpp
    src/posix/Thread.cpp
    src/posix/UdpSocket.cpp
    src/posix/UserKey.h
    src/posix/UserProtection.cpp
)
if(APPLE)
    # The platform's interface parts on macOS: accessibility settings, keychain secrets, the cursor, the front
    # window, the keyboard layout, input capture, speech, power, the menu bar item and USB.
    set(gs_platform_mac_sources
        src/mac/Accessibility.mm
        src/mac/Cursor.mm
        src/mac/Foreground.mm
        src/mac/InputCapture.mm
        src/mac/KeyboardLayout.mm
        src/mac/Speech.mm
        src/mac/StreamingPower.mm
        src/mac/TextReader.mm
        src/mac/TrayNotice.mm
        src/mac/Usb.mm
    )
    target_sources(gs_platform PRIVATE src/posix/UserKeyKeychain.cpp src/posix/UserKeyFile.cpp src/mac/Credentials.cpp
                   src/mac/KeyCodes.h ${gs_platform_mac_sources})
    set_source_files_properties(${gs_platform_mac_sources} PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    if(TARGET SDL3::SDL3)
        # Gamepads (InputCapture.mm) through SDL, as on Windows.
        target_link_libraries(gs_platform PRIVATE SDL3::SDL3)
        target_compile_definitions(gs_platform PRIVATE GS_HAVE_SDL3)
    endif()
    if(TARGET libusb::libusb)
        target_link_libraries(gs_platform PRIVATE libusb::libusb)
        target_compile_definitions(gs_platform PRIVATE GS_HAVE_LIBUSB)
    endif()
    target_link_libraries(gs_platform PRIVATE "-framework CoreFoundation" "-framework Security"
                          "-framework Foundation" "-framework CoreGraphics" "-framework Vision" "-framework AppKit"
                          "-framework Carbon" "-framework AVFoundation" "-framework Speech" "-framework UserNotifications"
                          "-framework IOKit" "-framework GameController" "-framework CoreHaptics")
else()
    target_sources(gs_platform PRIVATE src/posix/UserKeyFile.cpp src/posix/TextReaderNone.cpp)
endif()
add_library(gs::platform ALIAS gs_platform)

target_include_directories(gs_platform PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(gs_platform PUBLIC gs::base PRIVATE OpenSSL::Crypto Threads::Threads)
gs_apply_compiler_policy(gs_platform)
