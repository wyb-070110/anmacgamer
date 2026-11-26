qt_add_executable(anmacgamer MACOSX_BUNDLE
    main.cpp
    resources/GameScrcpy.icns
    resources/Assets.car
)
# The Dock, Finder and Launchpad icon, compiled from packaging/macos/icon/GameScrcpy.icon by
# packaging/macos/icon/build_icon.sh: Assets.car for macOS 26 and later, GameScrcpy.icns before.
set_source_files_properties(resources/GameScrcpy.icns resources/Assets.car PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
# anmacgamer.app, with its own bundle identifier.
set_target_properties(anmacgamer PROPERTIES
    MACOSX_BUNDLE_ICON_FILE "GameScrcpy"
    MACOSX_BUNDLE_BUNDLE_NAME "anmacgamer"
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.wyb070110.anmacgamer"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
)

qt_add_qml_module(anmacgamer
    URI Anmacgamer.App
    VERSION 1.0
    IMPORT_PATH "${QT_QML_OUTPUT_DIRECTORY}"
    QML_FILES
        qml/AboutPage.qml
        qml/AccentSetting.qml
        qml/ActionWheelView.qml
        qml/AiCorner.qml
        qml/AiDisclosureDialog.qml
        qml/AiModelPicker.qml
        qml/AiPanel.qml
        qml/AiSettings.qml
        qml/AiWindow.qml
        qml/AllowedDevices.qml
        qml/AllowedFolders.qml
        qml/AppsPage.qml
        qml/BatchBar.qml
        qml/CaptionButton.qml
        qml/CaptureAssignDialog.qml
        qml/CaptureCardSheet.qml
        qml/CaptureCropEditor.qml
        qml/CommandMenuItem.qml
        qml/BisectBar.qml
        qml/BundleDialog.qml
        qml/CommandPalette.qml
        qml/CompanionQrBlock.qml
        qml/ComponentSettings.qml
        qml/CopyToMineDialog.qml
        qml/CurveBox.qml
        qml/DeclaredParams.qml
        qml/DeviceCard.qml
        qml/DevicePanelSheet.qml
        qml/DevicesPage.qml
        qml/ExtensionCompatibilityDialog.qml
        qml/ExtensionLook.js
        qml/ExtensionSettingsSheet.qml
        qml/ExtensionsPage.qml
        qml/FilesPage.qml
        qml/FilePrompts.qml
        qml/FolderSetting.qml
        qml/Gallery.qml
        qml/GridOverlay.qml
        qml/GridHintLayer.qml
        qml/GuideHero.qml
        qml/GuideOverlay.qml
        qml/GuidePhone.qml
        qml/HardwarePicker.qml
        qml/HelpArticle.qml
        qml/HelpArtConnect.qml
        qml/HelpArtDriver.qml
        qml/HelpArtEditor.qml
        qml/HelpArtHome.qml
        qml/HelpArtMirror.qml
        qml/HelpArtProfile.qml
        qml/HelpArtScript.qml
        qml/HelpBlocks.qml
        qml/HelpFaq.qml
        qml/HelpFigure.qml
        qml/HelpHome.qml
        qml/HelpMarker.qml
        qml/HelpPage.qml
        qml/HelpPill.qml
        qml/HelpRich.js
        qml/HelpScene.qml
        qml/HelpScreen.qml
        qml/HelpSetupCard.qml
        qml/HelpSteps.qml
        qml/HelpText.qml
        qml/InstallDialog.qml
        qml/KeyFieldRow.qml
        qml/KeyHintLayer.qml
        qml/KeymapDraftLayer.qml
        qml/KeymapEditor.qml
        qml/KeyProperties.qml
        qml/LanguageSetting.qml
        qml/LatencyTestCard.qml
        qml/LazyPage.qml
        qml/LogicImportDialog.qml
        qml/LogicView.qml
        qml/LogicWindow.qml
        qml/MacMenuBar.qml
        qml/Main.qml
        qml/MaterialsList.qml
        qml/McpPrompts.qml
        qml/McpSettings.qml
        qml/MirrorOverlay.qml
        qml/NewComponentDialog.qml
        qml/NewProfileDialog.qml
        qml/NumberSetting.qml
        qml/PictureSourceSetting.qml
        qml/PlaceholderPage.qml
        qml/ProblemList.qml
        qml/ProfilesPage.qml
        qml/QrCodeView.qml
        qml/RecordingDialog.qml
        qml/RecordingsDialog.qml
        qml/RenameProfileDialog.qml
        qml/RestoreDialog.qml
        qml/SaveAsDialog.qml
        qml/ScriptDrawings.qml
        qml/ScriptPrompt.qml
        qml/ScriptWorkbenchEditor.qml
        qml/ScriptWorkbenchPanel.qml
        qml/ScriptWorkbenchSidebar.qml
        qml/ScriptWorkbenchTree.qml
        qml/SegmentsSetting.qml
        qml/SelectSetting.qml
        qml/SettingCard.qml
        qml/SettingRow.qml
        qml/SettingsPage.qml
        qml/SetupSteps.qml
        qml/ShortcutRow.qml
        qml/SliderSetting.qml
        qml/SpeechSettings.qml
        qml/StartPanel.qml
        qml/SwitchSetting.qml
        qml/SyncDialog.qml
        qml/TaskExampleDialog.qml
        qml/TaskSettings.qml
        qml/TasksPage.qml
        qml/TextSetting.qml
        qml/ThemeSetting.qml
        qml/TitleBar.qml
        qml/TourOverlay.qml
        qml/TouchMethodSetting.qml
        qml/VisionWorkbenchWindow.qml
        qml/WirelessSection.qml
        qml/ExportPackageDialog.qml
        qml/extui/UiCanvas.qml
        qml/extui/UiCardMenuItems.qml
        qml/extui/UiChart.qml
        qml/extui/UiContextMenu.qml
        qml/extui/UiList.qml
        qml/extui/UiMarkdown.js
        qml/extui/UiNavSection.qml
        qml/extui/UiNode.qml
        qml/extui/UiPage.qml
        qml/extui/UiPanelBlock.qml
        qml/extui/UiPanelView.qml
        qml/extui/UiSidePanel.qml
        qml/extui/UiStatusRow.qml
        qml/extui/UiTable.qml
        qml/extui/UiToolbarButton.qml
        qml/extui/UiToolbarPanel.qml
        qml/ConvertExportDialog.qml
        qml/ConvertMenuItems.qml
    SOURCES
        AiAssistant.cpp
        AiAssistant.h
        AiChats.cpp
        AiChats.h
        AiConversation.cpp
        AiConversation.h
        AiDrafts.cpp
        AiDrafts.h
        AiNetwork.cpp
        AiNetwork.h
        AiProviders.cpp
        AiProviders.h
        AiScripts.cpp
        AiScripts.h
        AiService.cpp
        AiService.h
        AoaSession.cpp
        AoaSession.h
        AoaTouchSink.cpp
        AoaTouchSink.h
        AppActions.cpp
        AppActions.h
        AppCatalog.cpp
        AppCatalog.h
        AppCommandHost.cpp
        AppCommandHost.h
        AppEvents.cpp
        AppEvents.h
        AppInfo.cpp
        AppInfo.h
        AppSettings.cpp
        AppSettings.h
        CaptureCards.cpp
        CaptureCards.h
        CaptureDemo.cpp
        CaptureDemo.h
        CaptureFeed.cpp
        CaptureFeed.h
        CaptureMatch.cpp
        CaptureMatch.h
        CaptureRow.h
        CommandMatcher.cpp
        CommandMatcher.h
        CommandModel.cpp
        CommandModel.h
        Commands.cpp
        Commands.h
        CommandState.cpp
        CommandState.h
        CommandTable.cpp
        CommandTable.h
        CompanionCipher.cpp
        CompanionCipher.h
        CompanionInput.cpp
        CompanionInput.h
        CompanionLan.cpp
        CompanionLan.h
        CompanionLink.cpp
        CompanionLink.h
        CompanionShots.cpp
        CompanionShots.h
        CompanionSync.cpp
        CompanionSync.h
        ComponentDefaults.cpp
        ComponentDefaults.h
        ComponentLibrary.cpp
        ComponentLibrary.h
        ControlChannel.cpp
        ControlChannel.h
        CrashReport.cpp
        CrashReport.h
        Declarations.cpp
        Declarations.h
        DemoFeed.cpp
        DemoFeed.h
        DeviceBatch.cpp
        DeviceBatch.h
        DeviceCommands.cpp
        DeviceCommands.h
        DebugRecord.cpp
        DebugRecord.h
        DeviceIdentifier.cpp
        DeviceIdentifier.h
        DeviceManager.cpp
        DeviceManager.h
        DevicePresentation.cpp
        DevicePresentation.h
        DiagnosticsBundle.cpp
        DiagnosticsBundle.h
        DiagnosticsOverlay.cpp
        DiagnosticsOverlay.h
        DraftDiff.cpp
        DraftDiff.h
        ExtensionBisect.cpp
        ExtensionBisect.h
        ExtensionHealth.cpp
        ExtensionHealth.h
        ExtensionHelp.cpp
        ExtensionHelp.h
        ExtensionIcons.cpp
        ExtensionIcons.h
        ExtensionHost.cpp
        ExtensionHost.h
        ExtensionLinks.cpp
        ExtensionLinks.h
        ExtensionUpdates.cpp
        ExtensionUpdates.h
        ExtensionLogic.cpp
        ExtensionLogic.h
        ExtensionMedia.cpp
        ExtensionMedia.h
        ExtensionModules.cpp
        ExtensionModules.h
        ExtensionPanels.cpp
        ExtensionPanels.h
        ExtensionProfile.h
        ExtensionServices.cpp
        ExtensionServices.h
        ExtensionTouchSink.cpp
        ExtensionTouchSink.h
        FailureAdvice.cpp
        FailureAdvice.h
        FileAccess.cpp
        FileAccess.h
        FileTransfers.cpp
        FileTransfers.h
        FilterBudget.cpp
        FilterBudget.h
        GamepadPassthrough.cpp
        GamepadPassthrough.h
        GridCells.cpp
        GridCells.h
        GridLayout.cpp
        GridLayout.h
        GridWindow.cpp
        GridWindow.h
        HardwareAccess.cpp
        HardwareAccess.h
        HardwareClaims.cpp
        HardwareClaims.h
        HelpLibrary.cpp
        HelpLibrary.h
        HelpMarkdown.cpp
        HelpMarkdown.h
        HidTouchscreen.cpp
        HidTouchscreen.h
        InputController.cpp
        InputController.h
        InputHost.cpp
        InputHost.h
        InputReadGrants.cpp
        InputReadGrants.h
        ItemOrder.cpp
        ItemOrder.h
        JsSequence.cpp
        JsSequence.h
        JsonFile.cpp
        JsonFile.h
        KeyCapture.cpp
        KeyCapture.h
        KeyForwarding.cpp
        KeyForwarding.h
        KeyHintModel.cpp
        KeyHintModel.h
        KeyLabels.cpp
        KeyLabels.h
        KeymapBackgrounds.cpp
        KeymapBackgrounds.h
        LanguagePacks.cpp
        LanguagePacks.h
        LatencyMeter.cpp
        LatencyMeter.h
        LocalServers.cpp
        LocalServers.h
        LogFile.cpp
        LogFile.h
        LogicReferences.cpp
        LogicReferences.h
        LogicServices.cpp
        LogicServices.h
        LogicUploads.cpp
        LogicUploads.h
        McpBlueprint.cpp
        McpBlueprint.h
        McpDevices.cpp
        McpDevices.h
        McpDrafts.cpp
        McpDrafts.h
        McpGuides.cpp
        McpGuides.h
        McpServer.cpp
        McpServer.h
        McpStarters.cpp
        McpStarters.h
        McpTools.cpp
        McpTools.h
        McpTrace.cpp
        McpTrace.h
        MediaFrames.cpp
        MediaFrames.h
        MediaOffers.h
        MediaSinks.cpp
        MediaSinks.h
        MirrorInputItem.cpp
        MirrorInputItem.h
        MirrorWindow.cpp
        MirrorWindow.h
        NativeEvents.cpp
        NativeEvents.h
        NativeExtensions.cpp
        NativeExtensions.h
        NativeJobs.cpp
        NativeJobs.h
        NetAccess.cpp
        NetAccess.h
        OfflineNetwork.cpp
        OfflineNetwork.h
        PhoneFiles.cpp
        PhoneFiles.h
        PhoneScreenWatch.cpp
        PhoneScreenWatch.h
        PhoneStatus.cpp
        PhoneStatus.h
        PhoneVendors.cpp
        PhoneVendors.h
        PictureTouch.cpp
        PreeditBubble.cpp
        PreeditBubble.h
        PictureTouch.h
        PictureZoom.cpp
        PictureZoom.h
        PointerMerge.cpp
        PointerMerge.h
        ProfileEditor.cpp
        ProfileEditor.h
        ProfileForm.cpp
        ProfileForm.h
        ProfileLibrary.cpp
        ProfileLibrary.h
        QrPicture.cpp
        QrPicture.h
        QtConversions.h
        RecordingConvert.cpp
        RecordingConvert.h
        Recordings.cpp
        Recordings.h
        ScriptHighlighter.cpp
        ScriptHighlighter.h
        ScriptRuntime.cpp
        ScriptRuntime.h
        ScriptWorkbench.cpp
        ScriptWorkbench.h
        ScriptWorkbenchApi.cpp
        ScriptWorkbenchApi.h
        ScriptWorkbenchConsole.cpp
        ScriptWorkbenchConsole.h
        ScriptWorkbenchFiles.cpp
        ScriptWorkbenchFiles.h
        SessionController.cpp
        SessionController.h
        SessionMetrics.cpp
        SessionMetrics.h
        Settings.cpp
        Settings.h
        SettingsCatalog.cpp
        SettingsCatalog.h
        SettingsSearch.cpp
        SettingsSearch.h
        SettingVariant.cpp
        SettingVariant.h
        SetupGuide.cpp
        SetupGuide.h
        SyncGroups.cpp
        SyncGroups.h
        SystemPanelService.cpp
        SystemPanelService.h
        TaskLibrary.cpp
        TaskLibrary.h
        ShortcutDispatcher.cpp
        ShortcutDispatcher.h
        ShortcutModel.cpp
        ShortcutModel.h
        SpeechEngines.cpp
        SpeechEngines.h
        ThemeTokens.h
        ThumbnailView.cpp
        ThumbnailView.h
        TouchBoxSession.cpp
        TouchBoxSession.h
        TouchMethods.h
        TranslateNoop.h
        TransportPhones.cpp
        TransportPhones.h
        TrayNotices.cpp
        TrayNotices.h
        UhidTouchSink.cpp
        UhidTouchSink.h
        UhidMouse.cpp
        UhidMouse.h
        UsbPhones.cpp
        UsbPhones.h
        VisionEngines.cpp
        VisionEngines.h
        VisionSearch.cpp
        VisionSearch.h
        VisionWorkbench.cpp
        VisionWorkbench.h
        WindowChrome.cpp
        WindowChrome.h
        WindowChromeMac.h
        WindowChromeMac.mm
        WindowFit.cpp
        WindowFit.h
        WirelessController.cpp
        WirelessController.h
        LanHosts.cpp
        LanHosts.h
        WebSocketClient.cpp
        WebSocketClient.h
        BlueprintLabels.cpp
        BlueprintLabels.h
        BlueprintModel.cpp
        BlueprintModel.h
        AmsiScan.cpp
        AmsiScan.h
        ExtensionContent.h
        ExtensionExport.cpp
        ExtensionExport.h
        Converters.cpp
        Converters.h
)

add_library(gs_profile_templates OBJECT)
qt_add_resources(gs_profile_templates "profile_templates"
    PREFIX "/templates"
    BASE templates
    FILES
        templates/action.json
        templates/blank.json
        templates/cards.json
        templates/moba.json
        templates/racing.json
        templates/shooter.json
)
target_link_libraries(gs_profile_templates PRIVATE Qt6::Core)

add_library(gs_help_articles OBJECT)
qt_add_resources(gs_help_articles "help_articles"
    PREFIX "/help"
    BASE help
    FILES
        help/index.json
        help/en/aoa-driver.md
        help/en/capture-card.md
        help/en/companion.md
        help/en/connecting.md
        help/en/extension-features.md
        help/en/extensions.md
        help/en/faq.md
        help/en/keymap-editor.md
        help/en/mirror-window.md
        help/en/profiles.md
        help/en/scripts.md
        help/en/touch-box.md
        help/en/tours.md
        help/zh_CN/aoa-driver.md
        help/zh_CN/capture-card.md
        help/zh_CN/companion.md
        help/zh_CN/connecting.md
        help/zh_CN/extension-features.md
        help/zh_CN/extensions.md
        help/zh_CN/faq.md
        help/zh_CN/keymap-editor.md
        help/zh_CN/mirror-window.md
        help/zh_CN/profiles.md
        help/zh_CN/scripts.md
        help/zh_CN/touch-box.md
        help/zh_CN/tours.md
)
file(GLOB gs_help_api_pages CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/sdk/docs/*.md")
qt_add_resources(gs_help_articles "help_api"
    PREFIX "/help/api"
    BASE "${PROJECT_SOURCE_DIR}/sdk/docs"
    FILES ${gs_help_api_pages}
)
target_link_libraries(gs_help_articles PRIVATE Qt6::Core)

target_link_libraries(anmacgamer PRIVATE
    gs::base
    gs::capture
    gs::ext
    gs::exthost
    gs::hardware
    gs::input
    gs::licenses
    gs::miniz
    gs::nlohmann_json
    gs::platform
    gs::qrcodegen
    gs::record
    gs::render
    gs::script
    gs::scripthost
    gs::session
    gs::tsstrip
    gs::vision
    gs_help_articles
    gs_profile_templates
    gs_ui_themeplugin
    gs_ui_iconsplugin
    gs_ui_controlsplugin
    gs_ui_blueprintplugin
    QWindowKit::Quick
    Qt6::Gui
    Qt6::Network
    Qt6::Qml
    Qt6::Quick
    gs::blueprint
)
target_compile_definitions(anmacgamer PRIVATE GS_BUILD_CONFIG="$<CONFIG>")
set_source_files_properties(WindowChromeMac.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
target_link_libraries(anmacgamer PRIVATE "-framework AppKit")
gs_apply_compiler_policy(anmacgamer)

# Qt's own texts in Chinese (the macOS application menu's Hide, Quit, Services…), built in like the app's.
set(gs_qt_translation "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_TRANSLATIONS}/qtbase_zh_CN.qm")
if(EXISTS "${gs_qt_translation}")
    qt_add_resources(anmacgamer gs_qt_translations PREFIX "/i18n/qt" BASE "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_TRANSLATIONS}"
                     FILES "${gs_qt_translation}")
endif()

qt_add_translations(anmacgamer
    TS_FILES i18n/anmacgamer_zh_CN.ts
    PLURALS_TS_FILE i18n/anmacgamer_en.ts
    SOURCES "${PROJECT_SOURCE_DIR}/sdk/i18n/api-texts.js"
    RESOURCE_PREFIX /i18n
    LUPDATE_OPTIONS -no-obsolete -locations none -tr-function-alias QT_TRANSLATE_NOOP+=GS_TRANSLATE_NOOP
)

# Helper executables live in Contents/MacOS; non-code data and the TypeScript library live in Contents/Resources so
# codesign can seal the bundle and bind Info.plist (needed for macOS Local Network and Notification permissions).
add_custom_target(gs_server_package ALL
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:anmacgamer>/Resources"
    COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE=${PROJECT_SOURCE_DIR}/android/server/build/outputs/apk/release/server-release-unsigned.apk"
        "-DDESTINATION=$<TARGET_BUNDLE_CONTENT_DIR:anmacgamer>/Resources/gs-server.apk"
        -P "${PROJECT_SOURCE_DIR}/cmake/CopyIfExists.cmake"
    COMMENT "Placing the server build inside anmacgamer.app/Contents/Resources"
    VERBATIM
)

add_custom_target(gs_companion_package ALL
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:anmacgamer>/Resources"
    COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE=${PROJECT_SOURCE_DIR}/android/companion/build/outputs/apk/debug/companion-debug.apk"
        "-DDESTINATION=$<TARGET_BUNDLE_CONTENT_DIR:anmacgamer>/Resources/gs-companion.apk"
        -P "${PROJECT_SOURCE_DIR}/cmake/CopyIfExists.cmake"
    COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE=${PROJECT_SOURCE_DIR}/android/companion/build/outputs/apk/debug/output-metadata.json"
        "-DDESTINATION=$<TARGET_BUNDLE_CONTENT_DIR:anmacgamer>/Resources/gs-companion.json"
        -P "${PROJECT_SOURCE_DIR}/cmake/CopyIfExists.cmake"
    COMMENT "Placing the companion app build inside anmacgamer.app/Contents/Resources"
    VERBATIM
)

add_custom_target(gs_builtin_components
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "$<TARGET_BUNDLE_CONTENT_DIR:anmacgamer>/Resources/components"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${PROJECT_SOURCE_DIR}/examples/components"
        "$<TARGET_BUNDLE_CONTENT_DIR:anmacgamer>/Resources/components"
    COMMENT "Placing the example components inside anmacgamer.app/Contents/Resources"
    VERBATIM
)

add_custom_target(gs_builtin_tasks
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "$<TARGET_BUNDLE_CONTENT_DIR:anmacgamer>/Resources/tasks"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${PROJECT_SOURCE_DIR}/examples/tasks"
        "$<TARGET_BUNDLE_CONTENT_DIR:anmacgamer>/Resources/tasks"
    COMMENT "Placing the example tasks inside anmacgamer.app/Contents/Resources"
    VERBATIM
)

# Built-in extensions (official, read-only): ready-made game profiles and the like.
add_custom_target(gs_builtin_extensions
    COMMAND "${CMAKE_COMMAND}" -E rm -rf "$<TARGET_BUNDLE_CONTENT_DIR:anmacgamer>/Resources/extensions"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/builtin-extensions"
        "$<TARGET_BUNDLE_CONTENT_DIR:anmacgamer>/Resources/extensions"
    COMMENT "Placing the built-in extensions inside anmacgamer.app/Contents/Resources"
    VERBATIM
)

# Signing. Ad hoc ("-") by default. macOS knows an ad-hoc signed app by the hash of its code, so every rebuild is a
# new program to the login keychain and asks for the password again. With a code-signing identity named
# "anmacgamer Local Signing" in the keychain (a self-signed certificate made on this Mac), builds are signed with it
# and keep one identity. GS_SIGN_IDENTITY overrides either.
set(GS_SIGN_IDENTITY "" CACHE STRING "codesign identity for anmacgamer.app; empty finds anmacgamer Local Signing or signs ad hoc")
set(gs_sign_identity "${GS_SIGN_IDENTITY}")
if(gs_sign_identity STREQUAL "")
    set(gs_sign_identity "-")
    execute_process(COMMAND /usr/bin/security find-identity -p codesigning
        OUTPUT_VARIABLE gs_identities ERROR_QUIET RESULT_VARIABLE gs_identities_result)
    if(gs_identities_result EQUAL 0 AND gs_identities MATCHES "([0-9A-F]+) \"anmacgamer Local Signing\"")
        set(gs_sign_identity "${CMAKE_MATCH_1}")
        message(STATUS "Signing anmacgamer.app with anmacgamer Local Signing (${gs_sign_identity})")
    endif()
endif()

add_dependencies(anmacgamer
    gs_builtin_extensions
    gs_exthost_program
    gs_scripthost_program
    gs_cli
    gs_typescript
    gs_server_package
    gs_companion_package
    gs_builtin_components
    gs_builtin_tasks
)
add_custom_command(TARGET anmacgamer POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E rm -rf
            "$<TARGET_FILE_DIR:anmacgamer>/components"
            "$<TARGET_FILE_DIR:anmacgamer>/tasks"
            "$<TARGET_FILE_DIR:anmacgamer>/typescript"
            "$<TARGET_FILE_DIR:anmacgamer>/gs-server.apk"
            "$<TARGET_FILE_DIR:anmacgamer>/gs-companion.apk"
            "$<TARGET_FILE_DIR:anmacgamer>/gs-companion.json"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:gs_exthost_program>"
            "$<TARGET_FILE:gs_scripthost_program>" "$<TARGET_FILE:gs_cli>" "$<TARGET_FILE_DIR:anmacgamer>"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory_if_different "${PROJECT_SOURCE_DIR}/third_party/typescript/lib"
            "$<TARGET_BUNDLE_CONTENT_DIR:anmacgamer>/Resources/typescript"
    COMMAND /usr/bin/codesign --force --sign "${gs_sign_identity}"
            "$<TARGET_FILE_DIR:anmacgamer>/gs-exthost"
            "$<TARGET_FILE_DIR:anmacgamer>/gs-scripthost"
            "$<TARGET_FILE_DIR:anmacgamer>/gs"
            "$<TARGET_BUNDLE_CONTENT_DIR:anmacgamer>/Resources/typescript/tsc"
    COMMAND /usr/bin/codesign --force --sign "${gs_sign_identity}" "$<TARGET_BUNDLE_DIR:anmacgamer>"
    COMMENT "Assembling and signing anmacgamer.app"
    VERBATIM
)

if(GS_BUILD_TESTS)
    add_test(NAME gs_app_smoke COMMAND anmacgamer --smoke-test)
    set_tests_properties(gs_app_smoke PROPERTIES TIMEOUT 60
        ENVIRONMENT "GS_DATA_DIR=${CMAKE_CURRENT_BINARY_DIR}/smoke-data"
    )
    add_test(NAME gs_app_smoke_mirror COMMAND anmacgamer --smoke-test
        --demo-device "${PROJECT_SOURCE_DIR}/tests/data/video/emulator_h264.media")
    set_tests_properties(gs_app_smoke_mirror PROPERTIES TIMEOUT 60
        ENVIRONMENT "GS_DATA_DIR=${CMAKE_CURRENT_BINARY_DIR}/smoke-mirror-data"
    )
    if(NOT GS_DEPLOY_QT_ON_BUILD)
        set_tests_properties(gs_app_smoke gs_app_smoke_mirror PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}"
        )
    endif()
    find_package(Python3 3.10 REQUIRED COMPONENTS Interpreter)
    add_test(NAME gs_qml_aot_lookups
        COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/tools/check_qml_aot.py" "${PROJECT_BINARY_DIR}/src")
endif()
