cmake_minimum_required(VERSION 3.20)
project(StreamEngine VERSION 0.1 LANGUAGES CXX)

# -----------------
# C++ Standard
# -----------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------
# Dependencies
# -----------------
include_directories(${CMAKE_SOURCE_DIR}/dependencies)
include_directories(${CMAKE_SOURCE_DIR}/headers)
include_directories(${CMAKE_SOURCE_DIR}/headers/network)

# Spdlog
add_subdirectory(dependencies/spdlog)
add_subdirectory(dependencies/boost)


# Nlohmann JSON
add_subdirectory(dependencies/json)

# -----------------
# Network
# -----------------
file(GLOB_RECURSE NETWORK_SRC "source/network/*.cpp")
add_library(network ${NETWORK_SRC})
target_include_directories(network PUBLIC headers/network ${Boost_INCLUDE_DIRS})
# Link network target to Boost libraries if available, otherwise link platform network libs
if(Boost_FOUND)
    target_link_libraries(network PUBLIC spdlog::spdlog ${Boost_LIBRARIES})
    target_link_libraries(nework PUBLIC Boost)
else()
    if (WIN32)
        target_link_libraries(network PUBLIC spdlog::spdlog ws2_32)
    else()
        target_link_libraries(network PUBLIC spdlog::spdlog)
    endif()
endif()

# -----------------
# Media
# -----------------
file(GLOB_RECURSE MEDIA_SRC "source/media/*.cpp")
add_library(media ${MEDIA_SRC})
target_include_directories(media PUBLIC headers/media ${BOOST_INCLUDEDIR})
target_link_libraries(media PUBLIC spdlog::spdlog)
target_link_libraries(media PUBLIC Boost)

# -----------------
# Streaming Library - DOSYALAR DIRECT SOURCE İÇİNDE
# -----------------
file(GLOB STREAMING_SRC 
    "source/publisher.cpp"
    "source/subscriber.cpp"
)
add_library(streaming ${STREAMING_SRC})
target_include_directories(streaming PUBLIC headers/streaming ${BOOST_INCLUDEDIR})
target_link_libraries(streaming PUBLIC network media spdlog::spdlog)
target_link_libraries(streaming PUBLIC Boost)


# -----------------
# Storage/VOD
# -----------------
file(GLOB_RECURSE STORAGE_SRC "source/storage/*.cpp")
add_library(storage ${STORAGE_SRC})
target_include_directories(storage PUBLIC headers/storage ${BOOST_INCLUDEDIR})
target_link_libraries(storage PUBLIC media spdlog::spdlog)
target_link_libraries(storage PUBLIC Boost)

# -----------------
# Control/API
# -----------------
file(GLOB_RECURSE CONTROL_SRC "source/control/*.cpp")
add_library(control ${CONTROL_SRC})
target_include_directories(control PUBLIC headers/control)
target_link_libraries(control PUBLIC streaming storage spdlog::spdlog nlohmann_json::nlohmann_json)
target_link_libraries(control PUBLIC Boost)

# -----------------
# Main executable
# -----------------
add_executable(stream_engine source/stream_engine.cpp)
target_include_directories(stream_engine PRIVATE headers)
target_link_libraries(stream_engine PRIVATE control)


# Define _WIN32_WINNT for Boost.Asio compatibility
add_definitions(-D_WIN32_WINNT=0x0A00)