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

# Spdlog
add_subdirectory(dependencies/spdlog)

# Nlohmann JSON
add_subdirectory(dependencies/json)

# Boost (header-only - dependencies/Boost klasöründen)
include_directories(${CMAKE_SOURCE_DIR}/dependencies/Boost)

# -----------------
# GRPC Configuration
# -----------------
set(GRPC_ROOT ${CMAKE_SOURCE_DIR}/dependencies/grpc)
if(EXISTS ${GRPC_ROOT})
    message(STATUS "GRPC found at: ${GRPC_ROOT}")
    
    # GRPC header include paths
    include_directories(${GRPC_ROOT}/include)
    
    # Find GRPC libraries
    set(GRPC_LIB_DIR ${GRPC_ROOT}/lib)
    
    # GRPC core libraries
    find_library(GRPC_LIB grpc PATHS ${GRPC_LIB_DIR} NO_DEFAULT_PATH)
    find_library(GRPCXX_LIB grpc++ PATHS ${GRPC_LIB_DIR} NO_DEFAULT_PATH)
    find_library(GRPC_UPB_LIB grpc_upb PATHS ${GRPC_LIB_DIR} NO_DEFAULT_PATH)
    find_library(GRPC_JSON_LIB grpc_json PATHS ${GRPC_LIB_DIR} NO_DEFAULT_PATH)
    
    # Protocol Buffers libraries (GRPC tarafından kullanılan)
    find_library(PROTOBUF_LIB protobuf PATHS ${GRPC_LIB_DIR} NO_DEFAULT_PATH)
    find_library(PROTOBUF_LITE_LIB protobuf-lite PATHS ${GRPC_LIB_DIR} NO_DEFAULT_PATH)
    
    # Additional GRPC libraries
    find_library(ADDRESS_SORTING_LIB address_sorting PATHS ${GRPC_LIB_DIR} NO_DEFAULT_PATH)
    find_library(CARES_LIB cares PATHS ${GRPC_LIB_DIR} NO_DEFAULT_PATH)
    find_library(RE2_LIB re2 PATHS ${GRPC_LIB_DIR} NO_DEFAULT_PATH)
    find_library(SSL_LIB ssl PATHS ${GRPC_LIB_DIR} NO_DEFAULT_PATH)
    find_library(CRYPTO_LIB crypto PATHS ${GRPC_LIB_DIR} NO_DEFAULT_PATH)
    find_library(ZLIB_LIB z PATHS ${GRPC_LIB_DIR} NO_DEFAULT_PATH)
    find_library(ABSL_LIBS 
        absl_bad_any_cast_impl
        absl_bad_optional_access
        absl_bad_variant_access
        absl_base
        absl_city
        absl_civil_time
        absl_cord
        absl_debugging_internal
        absl_demangle_internal
        absl_examine_stack
        absl_failure_signal_handler
        absl_flags
        absl_flags_commandlineflag
        absl_flags_config
        absl_flags_internal
        absl_flags_marshalling
        absl_flags_parse
        absl_flags_private_handle_accessor
        absl_flags_program_name
        absl_flags_reflection
        absl_flags_usage
        absl_flags_usage_internal
        absl_graphcycles_internal
        absl_hash
        absl_hashtablez_sampler
        absl_int128
        absl_leak_check
        absl_log_severity
        absl_malloc_internal
        absl_periodic_sampler
        absl_random_distributions
        absl_random_internal_distribution_test_util
        absl_random_internal_platform
        absl_random_internal_pool_urbg
        absl_random_internal_randen
        absl_random_internal_randen_hwaes
        absl_random_internal_randen_hwaes_impl
        absl_random_internal_randen_slow
        absl_random_internal_seed_material
        absl_random_seed_gen_exception
        absl_random_seed_sequences
        absl_raw_hash_set
        absl_raw_logging_internal
        absl_scoped_set_env
        absl_spinlock_wait
        absl_stacktrace
        absl_status
        absl_statusor
        absl_str_format_internal
        absl_strings
        absl_strings_internal
        absl_symbolize
        absl_synchronization
        absl_throw_delegate
        absl_time
        absl_time_zone
        PATHS ${GRPC_LIB_DIR} NO_DEFAULT_PATH
    )
    
    # GRPC library target oluştur
    add_library(grpc_libs INTERFACE)
    
    # GRPC kütüphanelerini ekle
    if(GRPC_LIB)
        target_link_libraries(grpc_libs INTERFACE ${GRPC_LIB})
        message(STATUS "Found GRPC library: ${GRPC_LIB}")
    endif()
    
    if(GRPCXX_LIB)
        target_link_libraries(grpc_libs INTERFACE ${GRPCXX_LIB})
        message(STATUS "Found GRPC++ library: ${GRPCXX_LIB}")
    endif()
    
    if(GRPC_UPB_LIB)
        target_link_libraries(grpc_libs INTERFACE ${GRPC_UPB_LIB})
    endif()
    
    if(GRPC_JSON_LIB)
        target_link_libraries(grpc_libs INTERFACE ${GRPC_JSON_LIB})
    endif()
    
    if(PROTOBUF_LIB)
        target_link_libraries(grpc_libs INTERFACE ${PROTOBUF_LIB})
        message(STATUS "Found Protobuf library: ${PROTOBUF_LIB}")
    endif()
    
    # Diğer bağımlılıklar
    if(ADDRESS_SORTING_LIB)
        target_link_libraries(grpc_libs INTERFACE ${ADDRESS_SORTING_LIB})
    endif()
    
    if(CARES_LIB)
        target_link_libraries(grpc_libs INTERFACE ${CARES_LIB})
    endif()
    
    if(RE2_LIB)
        target_link_libraries(grpc_libs INTERFACE ${RE2_LIB})
    endif()
    
    if(SSL_LIB)
        target_link_libraries(grpc_libs INTERFACE ${SSL_LIB})
    endif()
    
    if(CRYPTO_LIB)
        target_link_libraries(grpc_libs INTERFACE ${CRYPTO_LIB})
    endif()
    
    if(ZLIB_LIB)
        target_link_libraries(grpc_libs INTERFACE ${ZLIB_LIB})
    endif()
    
    # Compiler flags for GRPC
    target_compile_definitions(grpc_libs INTERFACE 
        GRPC_USE_PROTO_LITE
        USE_GRPC
    )
    
    # Platform-specific settings
    if(WIN32)
        target_link_libraries(grpc_libs INTERFACE ws2_32 crypt32)
        target_compile_definitions(grpc_libs INTERFACE 
            _WIN32_WINNT=0x0A00
            NOMINMAX
        )
    else()
        target_link_libraries(grpc_libs INTERFACE pthread dl m)
    endif()
    
    message(STATUS "GRPC configuration completed")
else()
    message(WARNING "GRPC not found at: ${GRPC_ROOT}")
endif()

# -----------------
# Protoc compiler path
# -----------------
set(PROTOC_PATH "${CMAKE_SOURCE_DIR}/dependencies/Protoc/bin/protoc.exe")
if(EXISTS ${PROTOC_PATH})
    message(STATUS "Protoc found at: ${PROTOC_PATH}")
else()
    message(WARNING "Protoc not found at: ${PROTOC_PATH}")
endif()

# -----------------
# Proto file compilation with GRPC support
# -----------------
file(GLOB_RECURSE PROTO_FILES "proto/*.proto")
if(PROTO_FILES AND EXISTS ${PROTOC_PATH})
    message(STATUS "Found proto files: ${PROTO_FILES}")
    
    # Generated headers için klasör oluştur
    set(PROTO_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    file(MAKE_DIRECTORY ${PROTO_GEN_DIR})
    
    # Include path'e generated headers'ı ekle
    include_directories(${PROTO_GEN_DIR})
    
    foreach(PROTO_FILE ${PROTO_FILES})
        get_filename_component(PROTO_DIR ${PROTO_FILE} DIRECTORY)
        get_filename_component(PROTO_NAME_WE ${PROTO_FILE} NAME_WE)
        
        # Generated .pb.cc ve .pb.h dosya yolları
        set(PB_CC "${PROTO_GEN_DIR}/${PROTO_NAME_WE}.pb.cc")
        set(PB_H "${PROTO_GEN_DIR}/${PROTO_NAME_WE}.pb.h")
        
        # GRPC generated files
        set(GRPC_PB_CC "${PROTO_GEN_DIR}/${PROTO_NAME_WE}.grpc.pb.cc")
        set(GRPC_PB_H "${PROTO_GEN_DIR}/${PROTO_NAME_WE}.grpc.pb.h")
        
        # Protoc komutunu çalıştır (hem proto hem grpc için)
        add_custom_command(
            OUTPUT ${PB_CC} ${PB_H} ${GRPC_PB_CC} ${GRPC_PB_H}
            COMMAND ${PROTOC_PATH} 
            ARGS --cpp_out=${PROTO_GEN_DIR} 
                 --grpc_out=${PROTO_GEN_DIR}
                 --plugin=protoc-gen-grpc=${GRPC_ROOT}/bin/grpc_cpp_plugin.exe
                 --proto_path=${PROTO_DIR} 
                 ${PROTO_FILE}
            DEPENDS ${PROTO_FILE}
            COMMENT "Generating C++ and GRPC code from ${PROTO_FILE}"
        )
        
        # Generated source'ları listeye ekle
        list(APPEND GENERATED_PROTO_SRCS ${PB_CC} ${GRPC_PB_CC})
        
        message(STATUS "Added proto compilation with GRPC: ${PROTO_FILE}")
    endforeach()
    
    # Generated proto dosyalarını library olarak ekle
    if(GENERATED_PROTO_SRCS)
        add_library(proto_generated ${GENERATED_PROTO_SRCS})
        target_include_directories(proto_generated PUBLIC ${PROTO_GEN_DIR})
        
        # GRPC bağımlılıklarını ekle
        if(TARGET grpc_libs)
            target_link_libraries(proto_generated PUBLIC grpc_libs)
        endif()
        
        message(STATUS "Proto generated library created with GRPC support")
    endif()
endif()

# -----------------
# Media (GRPC desteği ile)
# -----------------
file(GLOB_RECURSE MEDIA_SRC "source/media/*.cpp")
if(MEDIA_SRC)
    add_library(media ${MEDIA_SRC})
    target_include_directories(media PUBLIC headers/media)
    target_link_libraries(media PUBLIC spdlog::spdlog)
    
    # GRPC ve proto bağımlılıklarını ekle
    if(TARGET proto_generated)
        target_link_libraries(media PUBLIC proto_generated)
    endif()
    if(TARGET grpc_libs)
        target_link_libraries(media PUBLIC grpc_libs)
    endif()
endif()

# -----------------
# Network (GRPC desteği ile)
# -----------------
file(GLOB_RECURSE NETWORK_SRC "source/network/*.cpp")
if(NETWORK_SRC)
    add_library(network ${NETWORK_SRC})
    target_include_directories(network PUBLIC headers/network)
    target_link_libraries(network PUBLIC spdlog::spdlog)
    
    # GRPC ve proto bağımlılıklarını ekle
    if(TARGET proto_generated)
        target_link_libraries(network PUBLIC proto_generated)
    endif()
    if(TARGET grpc_libs)
        target_link_libraries(network PUBLIC grpc_libs)
    endif()
endif()

# -----------------
# Streaming Library (GRPC desteği ile)
# -----------------
file(GLOB STREAMING_SRC 
    "source/publisher.cpp"
    "source/subscriber.cpp"
)
if(STREAMING_SRC)
    add_library(streaming ${STREAMING_SRC})
    target_include_directories(streaming PUBLIC headers/streaming)
    target_link_libraries(streaming PUBLIC spdlog::spdlog)
    
    # GRPC ve proto bağımlılıklarını ekle
    if(TARGET proto_generated)
        target_link_libraries(streaming PUBLIC proto_generated)
    endif()
    if(TARGET grpc_libs)
        target_link_libraries(streaming PUBLIC grpc_libs)
    endif()
endif()

# -----------------
# Storage/VOD (GRPC desteği ile)
# -----------------
file(GLOB_RECURSE STORAGE_SRC "source/storage/*.cpp")
if(STORAGE_SRC)
    add_library(storage ${STORAGE_SRC})
    target_include_directories(storage PUBLIC headers/storage)
    target_link_libraries(storage PUBLIC spdlog::spdlog)
    if(TARGET media)
        target_link_libraries(storage PUBLIC media)
    endif()
    
    # GRPC ve proto bağımlılıklarını ekle
    if(TARGET proto_generated)
        target_link_libraries(storage PUBLIC proto_generated)
    endif()
    if(TARGET grpc_libs)
        target_link_libraries(storage PUBLIC grpc_libs)
    endif()
endif()

# -----------------
# Control/API (GRPC desteği ile)
# -----------------
file(GLOB_RECURSE CONTROL_SRC "source/control/*.cpp")
if(CONTROL_SRC)
    add_library(control ${CONTROL_SRC})
    target_include_directories(control PUBLIC headers/control)
    target_link_libraries(control PUBLIC spdlog::spdlog nlohmann_json::nlohmann_json)
    
    # GRPC ve proto bağımlılıklarını ekle
    if(TARGET proto_generated)
        target_link_libraries(control PUBLIC proto_generated)
    endif()
    if(TARGET grpc_libs)
        target_link_libraries(control PUBLIC grpc_libs)
    endif()
    
    # Optional dependencies
    if(TARGET streaming)
        target_link_libraries(control PUBLIC streaming)
    endif()
    if(TARGET storage)
        target_link_libraries(control PUBLIC storage)
    endif()
    if(TARGET media)
        target_link_libraries(control PUBLIC media)
    endif()
    if(TARGET network)
        target_link_libraries(control PUBLIC network)
    endif()
endif()

# -----------------
# Main executable (GRPC desteği ile)
# -----------------
add_executable(stream_engine source/stream_engine.cpp)
target_include_directories(stream_engine PRIVATE headers)

# Link libraries
target_link_libraries(stream_engine PRIVATE spdlog::spdlog)

# GRPC ve proto bağımlılıklarını ekle
if(TARGET proto_generated)
    target_link_libraries(stream_engine PRIVATE proto_generated)
endif()
if(TARGET grpc_libs)
    target_link_libraries(stream_engine PRIVATE grpc_libs)
endif()

# Optional: link control if it exists
if(TARGET control)
    target_link_libraries(stream_engine PRIVATE control)
endif()

# -----------------
# Platform-specific settings
# -----------------
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0A00)
    add_definitions(-DBOOST_ALL_NO_LIB)  # Boost auto-linking disable
    add_definitions(-DNOMINMAX)          # Windows min/max macro conflict
endif()

# -----------------
# Summary
# -----------------
message(STATUS "Project configured successfully")
message(STATUS "Using header-only Boost from dependencies/Boost")
if(EXISTS ${PROTOC_PATH})
    message(STATUS "Protoc available at: ${PROTOC_PATH}")
endif()
if(PROTO_FILES)
    message(STATUS "Proto files found: ${PROTO_FILES}")
endif()
if(EXISTS ${GRPC_ROOT})
    message(STATUS "GRPC integration enabled")
    message(STATUS "GRPC include path: ${GRPC_ROOT}/include")
    message(STATUS "GRPC library path: ${GRPC_ROOT}/lib")
endif()