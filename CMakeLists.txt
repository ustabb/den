# CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(StreamingEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# FFmpeg bağımlılıklarını bul
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED 
    libavcodec 
    libavformat 
    libavutil 
    libswscale
)

# Include directories
include_directories(include)
include_directories(${FFMPEG_INCLUDE_DIRS})

# Ana kütüphane
add_library(streaming_engine STATIC
    src/video/ffmpeg_wrapper.cpp
    src/video/video_decoder.cpp
    src/video/video_encoder.cpp
    src/video/frame_processor.cpp
    src/network/socket_manager.cpp
    src/processing/stream_batcher.cpp
    src/processing/bbr_turbo.cpp
)

target_link_libraries(streaming_engine 
    ${FFMPEG_LIBRARIES}
    pthread
    rt
)

# Örnek uygulama
add_executable(video_streaming_example examples/video_streaming_example.cpp)
target_link_libraries(video_streaming_example streaming_engine)