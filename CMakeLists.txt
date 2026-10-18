cmake_minimum_required(VERSION 3.18)
project(autoLyrica LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(NOMINMAX)

# Configure whisper.cpp options before adding subdirectory
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Use high-performance CPU AVX2 backend (CUDA 13 dropped support for sm_61 / GTX 1050 Ti)
set(GGML_CUDA OFF CACHE BOOL "" FORCE)
message(STATUS "Using Whisper CPU backend with AVX2/FMA vector optimization.")


# Add whisper.cpp
add_subdirectory(third_party/whisper.cpp)

# Dear ImGui library target
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Application Sources
set(APP_SOURCES
    src/main.cpp
    src/ui/app_ui.cpp
    src/ui/lyrics_view.cpp
    src/ui/theme.cpp
    src/audio/audio_decoder.cpp
    src/audio/audio_player.cpp
    src/transcriber/whisper_engine.cpp
    src/transcriber/model_manager.cpp
    src/exporter/lyric_exporter.cpp
)

add_executable(autoLyrica WIN32 ${APP_SOURCES})

target_include_directories(autoLyrica PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniaudio
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/whisper.cpp/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(autoLyrica PRIVATE
    imgui
    whisper
    d3d11
    dxgi
    d3dcompiler
    dwmapi
    wininet
    comdlg32
    ole32
    shell32
)

# Compiler specific options
if (MSVC)
    target_compile_options(autoLyrica PRIVATE /W3 /MP /utf-8)
else()
    target_compile_options(autoLyrica PRIVATE -Wall -Wextra)
endif()

# Automated Pipeline Test
add_executable(test_pipeline
    tests/test_pipeline.cpp
    src/audio/audio_decoder.cpp
    src/audio/audio_player.cpp
    src/transcriber/whisper_engine.cpp
    src/transcriber/model_manager.cpp
    src/exporter/lyric_exporter.cpp
)

target_include_directories(test_pipeline PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniaudio
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/whisper.cpp/include
)

target_link_libraries(test_pipeline PRIVATE
    whisper
    d3d11
    dxgi
    d3dcompiler
    dwmapi
    wininet
    comdlg32
    ole32
    shell32
)

if (MSVC)
    target_compile_options(test_pipeline PRIVATE /W3 /MP /utf-8)
endif()

