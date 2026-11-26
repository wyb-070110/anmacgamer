# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Yubo Wu and the anmacgamer contributors
# Port probe: unit tests of the Windows-free modules.
function(gs_add_unit_test name)
    cmake_parse_arguments(PARSE_ARGV 1 arg "" "" "SOURCES;LIBRARIES;ARGUMENTS")
    add_executable(${name} unit/main.cpp ${arg_SOURCES})
    target_link_libraries(${name} PRIVATE gs::doctest ${arg_LIBRARIES})
    gs_apply_compiler_policy(${name})
    add_test(NAME ${name} COMMAND ${name} ${arg_ARGUMENTS})
endfunction()

gs_add_unit_test(gs_base_tests
    SOURCES
        unit/base/BuildInfoTests.cpp
        unit/base/ByteOrderTests.cpp
        unit/base/ClockTests.cpp
        unit/base/ExpectedTests.cpp
        unit/base/FromCharsTests.cpp
        unit/base/InplaceVectorTests.cpp
        unit/base/JsonNestingTests.cpp
        unit/base/LogTests.cpp
        unit/base/SpscQueueTests.cpp
        unit/base/ThreadCheckerTests.cpp
    LIBRARIES
        gs::base
)
target_compile_definitions(gs_base_tests PRIVATE GS_EXPECTED_VERSION="${PROJECT_VERSION}")

gs_add_unit_test(gs_protocol_tests
    SOURCES
        unit/protocol/DatagramTests.cpp
        unit/protocol/DirectMediaTests.cpp
        unit/protocol/GoldenVectorTests.cpp
        unit/protocol/JsonVisitors.h
        unit/protocol/ReedSolomonTests.cpp
        unit/protocol/ReliableTests.cpp
        unit/protocol/RobustnessTests.cpp
    LIBRARIES
        gs::protocol
        gs::nlohmann_json
)
target_compile_definitions(gs_protocol_tests PRIVATE
    GS_PROTOCOL_VECTOR_DIR="${PROJECT_SOURCE_DIR}/protocol/vectors"
)
target_include_directories(gs_protocol_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

gs_add_unit_test(gs_input_tests
    SOURCES
        unit/input/ControlsTests.cpp
        unit/input/DeviceEventsTests.cpp
        unit/input/EngineTests.cpp
        unit/input/ExtensionInputTests.cpp
        unit/input/GeometryTests.cpp
        unit/input/InputSourceTests.cpp
        unit/input/KeyHintsTests.cpp
        unit/input/ModuleTests.cpp
        unit/input/PadProfileTests.cpp
        unit/input/PadReportsTests.cpp
        unit/input/PointerAllocatorTests.cpp
        unit/input/ProfileDiffTests.cpp
        unit/input/ProfilePatchTests.cpp
        unit/input/ProfileTests.cpp
        unit/input/TouchFrameBuilderTests.cpp
        unit/input/TriggerStateTests.cpp
    LIBRARIES
        gs::input
)
target_sources(gs_input_tests PRIVATE unit/input/AcceptanceTests.cpp)

gs_add_unit_test(gs_tsstrip_tests
    SOURCES
        unit/tsstrip/StripTests.cpp
        unit/tsstrip/TokensTests.cpp
    LIBRARIES
        gs::tsstrip
)
target_include_directories(gs_tsstrip_tests PRIVATE "${PROJECT_SOURCE_DIR}/src/tsstrip/src")

gs_add_unit_test(gs_vision_tests
    SOURCES
        unit/vision/BenchmarkTests.cpp
        unit/vision/ColorTests.cpp
        unit/vision/FixtureTests.cpp
        unit/vision/ImageTests.cpp
        unit/vision/MatcherTests.cpp
        unit/vision/TestImages.h
    LIBRARIES
        gs::vision
)
target_compile_definitions(gs_vision_tests PRIVATE
    GS_VISION_TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/tests/data/vision"
)

# Platform, adb and session: the POSIX backend under src/platform/src/posix. The test fakes speak Winsock,
# which compat/posix maps onto BSD sockets.
gs_add_unit_test(gs_platform_tests
    SOURCES
        unit/platform/CryptoTests.cpp
        unit/platform/UdpSocketTests.cpp
        unit/platform/posix/PosixPlatformTests.cpp
    LIBRARIES
        gs::platform
)
if(APPLE)
    target_sources(gs_platform_tests PRIVATE unit/platform/mac/SpeechMacTests.mm unit/platform/mac/TextReaderMacTests.mm
                   unit/platform/mac/InputCaptureMacTests.mm unit/platform/mac/UsbMacTests.mm)
    set_source_files_properties(unit/platform/mac/SpeechMacTests.mm unit/platform/mac/TextReaderMacTests.mm
                                unit/platform/mac/InputCaptureMacTests.mm unit/platform/mac/UsbMacTests.mm
                                PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    target_link_libraries(gs_platform_tests PRIVATE "-framework Foundation" "-framework CoreGraphics"
                          "-framework CoreText" "-framework AppKit" "-framework Speech")
endif()

gs_add_unit_test(gs_adb_tests
    SOURCES
        unit/adb/AcceptanceAdbTests.cpp
        unit/adb/AdbClientTests.cpp
        unit/adb/DeviceIdentityTests.cpp
        unit/adb/DeviceListTests.cpp
        unit/adb/DeviceTrackerTests.cpp
        unit/adb/FakeAdbServer.h
        unit/adb/ForwardListTests.cpp
        unit/adb/ServerLauncherTests.cpp
        unit/adb/ShellProtocolTests.cpp
        unit/adb/WirelessServicesTests.cpp
    LIBRARIES
        gs::adb
)
target_include_directories(gs_adb_tests BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/compat/posix")

gs_add_unit_test(gs_session_tests
    SOURCES
        fakes/FakeDeviceServer.h
        fakes/FakeServerHost.h
        fakes/RecordingListener.h
        unit/adb/FakeAdbServer.h
        unit/session/AcceptanceSessionTests.cpp
        unit/session/AdbServerHostTests.cpp
        unit/session/CompanionMirrorTests.cpp
        unit/session/DatagramVectors.h
        unit/session/DirectHandshakeTests.cpp
        unit/session/DirectTransportTests.cpp
        unit/session/HandshakeTests.cpp
        unit/session/KeyFrameGateTests.cpp
        unit/session/ReceiverStatsTests.cpp
        unit/session/ReliableStreamTests.cpp
        unit/session/SendDelayStatsTests.cpp
        unit/session/SessionTests.cpp
        unit/session/StreamTransportTests.cpp
    LIBRARIES
        gs::session
        gs::nlohmann_json
)
target_include_directories(gs_session_tests BEFORE PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/compat/posix"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/src/session/src"
)
target_compile_definitions(gs_session_tests PRIVATE
    GS_PROTOCOL_VECTOR_DIR="${PROJECT_SOURCE_DIR}/protocol/vectors"
    GS_SERVER_PACKAGE_FILE="${PROJECT_SOURCE_DIR}/android/server/build/outputs/apk/release/server-release-unsigned.apk"
)

gs_add_unit_test(gs_companion_cipher_tests
    SOURCES
        unit/app/CompanionMirrorCipherTests.cpp
        "${PROJECT_SOURCE_DIR}/src/app/CompanionCipher.cpp"
    LIBRARIES
        gs::session
        gs::nlohmann_json
)
target_include_directories(gs_companion_cipher_tests BEFORE PRIVATE "${PROJECT_SOURCE_DIR}/src/app")
target_compile_definitions(gs_companion_cipher_tests PRIVATE
    GS_PROTOCOL_VECTOR_DIR="${PROJECT_SOURCE_DIR}/protocol/vectors"
)

if(TARGET Qt6::Network)
    gs_add_unit_test(gs_companion_lan_tests
        SOURCES
            unit/app/CompanionLanMacTests.cpp
            "${PROJECT_SOURCE_DIR}/src/app/CompanionCipher.cpp"
            "${PROJECT_SOURCE_DIR}/src/app/CompanionLan.cpp"
            "${PROJECT_SOURCE_DIR}/src/app/CompanionLink.cpp"
            "${PROJECT_SOURCE_DIR}/src/app/CompanionShots.cpp"
        LIBRARIES
            gs::input
            gs::nlohmann_json
            gs::session
            Qt6::Core
            Qt6::Network
    )
    target_include_directories(gs_companion_lan_tests BEFORE PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/compat/posix"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/src/app"
    )
endif()

# Device tests: they run against a real phone and report "skipped" (77) without GS_ADB_TEST_SERIAL.
add_executable(gs_adb_device_tests integration/adb/AdbDeviceTests.cpp)
target_link_libraries(gs_adb_device_tests PRIVATE gs::adb gs::platform gs::doctest)
target_include_directories(gs_adb_device_tests BEFORE PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/compat/posix")
gs_apply_compiler_policy(gs_adb_device_tests)
add_test(NAME gs_adb_device_tests COMMAND gs_adb_device_tests)
set_tests_properties(gs_adb_device_tests PROPERTIES SKIP_RETURN_CODE 77 LABELS device)

add_executable(gs_session_device_tests integration/session/SessionDeviceTests.cpp)
target_link_libraries(gs_session_device_tests PRIVATE gs::session gs::doctest gs::miniz)
target_include_directories(gs_session_device_tests BEFORE PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/compat/posix"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_definitions(gs_session_device_tests PRIVATE
    GS_DEFAULT_SERVER_APK="${PROJECT_SOURCE_DIR}/android/server/build/outputs/apk/release/server-release-unsigned.apk"
)
gs_apply_compiler_policy(gs_session_device_tests)
add_test(NAME gs_session_device_tests COMMAND gs_session_device_tests)
set_tests_properties(gs_session_device_tests PROPERTIES SKIP_RETURN_CODE 77 LABELS device)

# Scripts and extensions. Native extensions are Windows DLLs, so their checks (NativeCheckTests and the
# test DLLs) stay Windows-only for now.
gs_add_unit_test(gs_ext_tests
    SOURCES
        unit/ext/ArchiveTests.cpp
        unit/ext/BundleTests.cpp
        unit/ext/FilterSourceTests.cpp
        unit/ext/MachOTests.cpp
        unit/ext/ManifestTests.cpp
        unit/ext/PackageTests.cpp
        unit/ext/ResolutionTests.cpp
        unit/ext/TestFiles.h
        unit/ext/VersionTests.cpp
        unit/ext/MinisignTests.cpp
        unit/ext/SchemaTests.cpp
        unit/ext/SigningTests.cpp
        unit/ext/UpdateIndexTests.cpp
    LIBRARIES
        gs::ext
        gs::miniz
        gs::nlohmann_json
)
target_compile_definitions(gs_ext_tests PRIVATE GS_EXAMPLES_DIR="${PROJECT_SOURCE_DIR}/examples")
if(APPLE)
    # Real dylibs for the Mach-O checks: one that only opens files, one that also imports network, process
    # and injection functions.
    add_library(gs_test_macho_clean SHARED fakes/native/MachOProbe.cpp)
    add_library(gs_test_macho_probe SHARED fakes/native/MachOProbe.cpp)
    target_compile_definitions(gs_test_macho_probe PRIVATE GS_PROBE_EVERYTHING)
    # The same probe in the older format, bind opcodes in LC_DYLD_INFO instead of chained fixups.
    add_library(gs_test_macho_probe_classic SHARED fakes/native/MachOProbe.cpp)
    target_compile_definitions(gs_test_macho_probe_classic PRIVATE GS_PROBE_EVERYTHING)
    target_link_options(gs_test_macho_probe_classic PRIVATE "LINKER:-no_fixup_chains")
    target_compile_definitions(gs_ext_tests PRIVATE
        GS_TEST_MACHO_CLEAN="$<TARGET_FILE:gs_test_macho_clean>"
        GS_TEST_MACHO_PROBE="$<TARGET_FILE:gs_test_macho_probe>"
        GS_TEST_MACHO_PROBE_CLASSIC="$<TARGET_FILE:gs_test_macho_probe_classic>")
    add_dependencies(gs_ext_tests gs_test_macho_clean gs_test_macho_probe gs_test_macho_probe_classic)
    # The checks read one-architecture files (and wrap one into a universal file themselves), whatever the app is
    # built for: the probes are built for this Mac's processor only.
    set_target_properties(gs_test_macho_clean gs_test_macho_probe gs_test_macho_probe_classic PROPERTIES
        OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}")
endif()

gs_add_unit_test(gs_script_tests
    SOURCES
        unit/script/AiAskTests.cpp
        unit/script/ExtensionNodeTests.cpp
        unit/script/ExtensionServiceTests.cpp
        unit/script/HardwareApiTests.cpp
        unit/script/InputReadTests.cpp
        unit/script/LoadMeterTests.cpp
        unit/script/LogicHostTests.cpp
        unit/script/MediaApiTests.cpp
        unit/script/ModulePathTests.cpp
        unit/script/NativeHeaderTests.cpp
        unit/script/NetFilesApiTests.cpp
        unit/script/ReportTests.cpp
        unit/script/ResourcesTests.cpp
        unit/script/ScriptApiTests.cpp
        unit/script/ScriptFixture.h
        unit/script/ScriptHostTests.cpp
        unit/script/ScriptTimingTests.cpp
        unit/script/SessionTests.cpp
        unit/script/StackTraceTests.cpp
        unit/script/SystemPanelApiTests.cpp
        unit/script/TestRunTests.cpp
        unit/script/TextSearchTests.cpp
        unit/script/UiPanelTests.cpp
        unit/script/VisionEngineTests.cpp
        unit/script/WorldModelTests.cpp
    LIBRARIES
        gs::script
        gs::platform
        gs::nlohmann_json
)
# Runners whose sleeps wake late (the macOS CI virtual machines: a 4 ms sleep wakes about 14 ms late) set
# GS_COARSE_TIMERS; the two cases that measure timer precision then do not run there.
if(DEFINED ENV{GS_COARSE_TIMERS})
    add_test(NAME gs_script_tests_coarse_timers COMMAND gs_script_tests
             "--test-case-exclude=*lands?close?to?its?deadline*,*steady?load?warns*")
    set_tests_properties(gs_script_tests PROPERTIES DISABLED ON)
endif()
target_include_directories(gs_script_tests BEFORE PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/compat/posix"
    "${PROJECT_SOURCE_DIR}/src/script/src"
)
target_compile_definitions(gs_script_tests PRIVATE
    GS_SDK_DIR="${PROJECT_SOURCE_DIR}/sdk"
    GS_EXAMPLE_COMPONENTS_DIR="${PROJECT_SOURCE_DIR}/examples/components"
)

# Script logic in its own process (gs-scripthost). Linux has no sandbox for it, so there it runs unsandboxed
# for the tests only and the sandbox self-test is left out; macOS runs everything in the sandbox.
gs_add_unit_test(gs_scripthost_tests
    SOURCES
        unit/script/ScriptFixture.h
        unit/scripthost/RemoteLogicHostTests.cpp
    LIBRARIES
        gs::scripthost
        gs::script
    ARGUMENTS
        $<$<NOT:$<PLATFORM_ID:Darwin>>:--test-case-exclude=*cannot?open?the?user*>
)
target_include_directories(gs_scripthost_tests BEFORE PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/compat/posix"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_definitions(gs_scripthost_tests PRIVATE
    GS_SCRIPTHOST_PROGRAM="$<TARGET_FILE:gs_scripthost_program>"
)
add_dependencies(gs_scripthost_tests gs_scripthost_program)
if(NOT APPLE)
    set_tests_properties(gs_scripthost_tests PROPERTIES ENVIRONMENT "GS_SCRIPTHOST_UNSANDBOXED=1")
endif()

# Media: decoding (VideoToolbox on macOS), snapshots, sound. The Direct3D 11 cases are Windows-only; the
# POSIX hardware tests compare VideoToolbox pictures with software ones.
gs_add_unit_test(gs_media_tests
    SOURCES
        unit/media/AudioPlayerTests.cpp
        unit/media/AudioRegulatorTests.cpp
        unit/media/ColorMatrixTests.cpp
        unit/media/FfmpegLogTests.cpp
        unit/media/FrameSnapshotTests.cpp
        unit/media/FrameUploaderTests.cpp
        unit/media/LumaSamplerTests.cpp
        unit/media/PendingArrivalsTests.cpp
        unit/media/ThumbnailerTests.cpp
        unit/media/VideoDecoderTests.cpp
        unit/media/posix/HardwareFrameTests.cpp
    LIBRARIES
        gs::media
        gs::vision
        FFmpeg::avcodec
        FFmpeg::swresample
)
target_compile_definitions(gs_media_tests PRIVATE GS_VIDEO_TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/tests/data/video")
target_include_directories(gs_media_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/src/media/src"
)

# Recording: the recorder, operations, MP4 export and replay (the pictures need ImageIO, so Linux skips them).
add_executable(gs_recorder_child unit/media/RecorderChild.cpp compat/posix/WideMain.cpp)
target_link_libraries(gs_recorder_child PRIVATE gs::record)
gs_apply_compiler_policy(gs_recorder_child)

gs_add_unit_test(gs_record_tests
    SOURCES
        unit/media/AacRecordingTests.cpp
        unit/media/RecordingTests.cpp
    LIBRARIES
        gs::media
        gs::record
        gs::script
        gs::vision
        FFmpeg::avcodec
        FFmpeg::avformat
        FFmpeg::swresample
    ARGUMENTS
        $<$<NOT:$<PLATFORM_ID:Darwin>>:--test-case-exclude=*replaying?a?recording*>
)
target_compile_definitions(gs_record_tests PRIVATE
    GS_VIDEO_TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/tests/data/video"
    GS_RECORDER_CHILD="$<TARGET_FILE:gs_recorder_child>"
)
target_include_directories(gs_record_tests BEFORE PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/compat/posix"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/src/media/src"
)
add_dependencies(gs_record_tests gs_recorder_child)

# Capture cards: the format, crop, pairing and BGRA logic (the capture itself is not ported yet).
gs_add_unit_test(gs_capture_tests
    SOURCES
        unit/capture/AudioPairingTests.cpp
        unit/capture/BgraFramesTests.cpp
        unit/capture/CaptureFormatTests.cpp
        unit/capture/CropDetectorTests.cpp
    LIBRARIES
        gs::capture
        gs::media
        gs::protocol
)
target_compile_definitions(gs_capture_tests PRIVATE GS_CAPTURE_TEST_DATA_DIR="${PROJECT_SOURCE_DIR}/tests/data/capture")

# Hardware for scripts: device ids and names, the fake backend, and the macOS IOKit/termios serial backend.
gs_add_unit_test(gs_hardware_tests
    SOURCES
        unit/hardware/HardwareTests.cpp
        $<$<PLATFORM_ID:Darwin>:unit/hardware/mac/SerialBackendTests.cpp>
        $<$<NOT:$<PLATFORM_ID:Darwin>>:unit/hardware/posix/NoDevicesBackendTests.cpp>
    LIBRARIES
        gs::hardware
)

# Native extensions: the test DLLs as macOS libraries (their Win32 calls mapped as in gs-exthost), and the mock host
# of gs ext test running them in this process and in gs-exthost.
add_library(gs_test_native SHARED fakes/native/TestNative.cpp
    "${PROJECT_SOURCE_DIR}/src/exthost/src/Channel.cpp"
    "${PROJECT_SOURCE_DIR}/src/exthost/src/posix/Compat.cpp")
target_include_directories(gs_test_native PRIVATE "${PROJECT_SOURCE_DIR}/sdk/include"
    "${PROJECT_SOURCE_DIR}/src/exthost/src")
target_include_directories(gs_test_native BEFORE PRIVATE "${PROJECT_SOURCE_DIR}/src/exthost/src/posix/compat")
target_link_libraries(gs_test_native PRIVATE ${CMAKE_DL_LIBS})
gs_apply_compiler_policy(gs_test_native)

add_library(gs_test_native_old SHARED fakes/native/TestNativeOld.cpp)
gs_apply_compiler_policy(gs_test_native_old)

gs_add_unit_test(gs_mockhost_tests
    SOURCES
        unit/cli/MockHostTests.cpp
    LIBRARIES
        gs_mockhost
        gs::ext
        gs::nlohmann_json
)
target_compile_definitions(gs_mockhost_tests PRIVATE
    GS_EXT_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/ext-test"
    GS_TEST_NATIVE_DLL="$<TARGET_FILE:gs_test_native>"
    GS_TEST_NATIVE_OLD_DLL="$<TARGET_FILE:gs_test_native_old>"
    GS_EXTHOST_PROGRAM="$<TARGET_FILE:gs_exthost_program>"
)
add_dependencies(gs_mockhost_tests gs_test_native gs_test_native_old gs_exthost_program)

# gs-exthost on macOS: shared memory to and from the child, a crash and its report, and the sandbox of a restricted DLL.
gs_add_unit_test(gs_exthost_tests
    SOURCES
        unit/exthost/posix/HostedTests.cpp
    LIBRARIES
        gs::exthost
        gs::nlohmann_json
)
target_compile_definitions(gs_exthost_tests PRIVATE
    GS_TEST_NATIVE_DLL="$<TARGET_FILE:gs_test_native>"
    GS_EXTHOST_PROGRAM="$<TARGET_FILE:gs_exthost_program>"
    GS_EXTHOST_TESTS_PROGRAM="$<TARGET_FILE:gs_exthost_tests>"
)
add_dependencies(gs_exthost_tests gs_test_native gs_exthost_program)

# Rendering on macOS (built with the app): how frames are timed to the screen.
if(TARGET gs::render)
    gs_add_unit_test(gs_render_tests
        SOURCES
            unit/render/FrameTimingTests.cpp
        LIBRARIES
            gs::render
    )
endif()

# Qt Quick controls on macOS (FloatingToolbar, ToolbarSeparator, ToolbarRevealArea and the rest of ui/controls).
if(TARGET gs_ui_controlsplugin)
    find_package(Qt6 6.10 REQUIRED COMPONENTS QuickTest)
    add_executable(gs_controls_tests ui/controls/ControlsTests.cpp ui/controls/tst_controls.qml)
    target_link_libraries(gs_controls_tests PRIVATE
        Qt6::QuickTest
        gs_ui_themeplugin
        gs_ui_iconsplugin
        gs_ui_controlsplugin
    )
    target_compile_definitions(gs_controls_tests PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/ui/controls"
    )
    gs_apply_compiler_policy(gs_controls_tests)
    foreach(suite IN ITEMS floating_toolbar toolbar_reveal)
        add_test(NAME gs_${suite}_tests
            COMMAND gs_controls_tests -platform offscreen
                -import "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_QML}"
                -input "${CMAKE_CURRENT_SOURCE_DIR}/ui/controls/tst_${suite}.qml")
        set_tests_properties(gs_${suite}_tests PROPERTIES ENVIRONMENT_MODIFICATION
            "QT_PLUGIN_PATH=set:${QT6_INSTALL_PREFIX}/${QT6_INSTALL_PLUGINS};QT_FORCE_STDERR_LOGGING=set:1")
    endforeach()
endif()
