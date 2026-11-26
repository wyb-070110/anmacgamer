# gs_render: the video item of Qt Quick (PLAN 4.5), drawn with QRhi on Metal.

qt_add_library(gs_render STATIC)
qt_add_qml_module(gs_render
    URI Anmacgamer.Render
    VERSION 1.0
    SOURCES
        include/gs/render/FrameSlot.h
        include/gs/render/GraphicsDevice.h
        include/gs/render/VideoFilter.h
        include/gs/render/VideoItem.h
        src/FilterChain.cpp
        src/FilterChain.h
        src/FrameSlot.cpp
        src/GraphicsDevice.cpp
        src/PresentClock.cpp
        src/PresentClock.h
        src/VideoGeometry.h
        src/VideoItem.cpp
        src/VideoRenderNode.cpp
        src/VideoRenderNode.h
)
add_library(gs::render ALIAS gs_render)

target_include_directories(gs_render
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include/gs/render"
)
target_link_libraries(gs_render
    PUBLIC gs::media Qt6::Quick
    PRIVATE gs::ext Qt6::GuiPrivate "-framework CoreVideo"
)
gs_apply_compiler_policy(gs_render)

find_package(Qt6 6.10 REQUIRED COMPONENTS ShaderTools)
qt_add_shaders(gs_render "gs_render_shaders"
    PREFIX "/gs/render"
    FILES
        shaders/video.vert
        shaders/video_nv12.frag
        shaders/video_planar.frag
)
