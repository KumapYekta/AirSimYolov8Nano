cmake_minimum_required(VERSION 3.16)

project(AirSimYolov8Nano
    VERSION 0.1.0
    DESCRIPTION "AirSim camera capture pipeline for Jetson Orin Nano"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# -----------------------------------------------------------------------------
# Dizin yapısı
#
#   <workspace>/
#     ├── cots/                 -> 3. parti bagımlılıklar (AirSim, TensorRT sarmalayıcıları, ...)
#     │     └── AirSim/
#     └── projects/
#           └── AirSimYolov8Nano/   -> bu proje
#
# Once environment variable'a bakılır, yoksa gorece yol kullanılır.
# -----------------------------------------------------------------------------

if(DEFINED ENV{COTS_ROOT})
    set(COTS_ROOT "$ENV{COTS_ROOT}" CACHE PATH "Third party dependency root")
else()
    get_filename_component(COTS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../cots" ABSOLUTE)
    set(COTS_ROOT "${COTS_ROOT}" CACHE PATH "Third party dependency root")
endif()

if(DEFINED ENV{AIRSIM_ROOT})
    set(AIRSIM_ROOT "$ENV{AIRSIM_ROOT}" CACHE PATH "AirSim root directory")
else()
    set(AIRSIM_ROOT "${COTS_ROOT}/AirSim" CACHE PATH "AirSim root directory")
endif()

message(STATUS "COTS_ROOT   : ${COTS_ROOT}")
message(STATUS "AIRSIM_ROOT : ${AIRSIM_ROOT}")

if(NOT EXISTS "${COTS_ROOT}")
    message(FATAL_ERROR "COTS_ROOT not found: ${COTS_ROOT} (export COTS_ROOT or pass -DCOTS_ROOT=<path>)")
endif()

# Header ve Library yollarini dinamik ekleme
include_directories(
    ${AIRSIM_ROOT}/AirLib/include
    ${AIRSIM_ROOT}/AirLib/deps/rpclib/include
    ${AIRSIM_ROOT}/AirLib/deps/eigen3
)

add_executable(${PROJECT_NAME}
    main.cpp
    AirSimConnectionManager.cpp
    AirSimConnectionManager.h
    AirSimIncludes.h
    CameraFrame.h
    CameraImageProvider.cpp
    CameraImageProvider.h
    FrameStatisticsManager.cpp
    FrameStatisticsManager.h
    ImageRequestProvider.cpp
    ImageRequestProvider.h
    .gitignore
    .gitattributes
    # varsa diğer .cpp ve .h dosyalarınız
)

target_link_directories(${PROJECT_NAME} PRIVATE
    "${AIRSIM_ROOT}/AirLib/lib/x64/$<CONFIG>"
    "${AIRSIM_ROOT}/AirLib/deps/rpclib/lib/x64/$<CONFIG>"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        AirLib
        rpc
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /std:c++17 /permissive-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -std=c++17)
endif()

# AirLib clang + libc++ ile derlendiyse (AirSim'in kendi build.sh varsayılanı)
# uygulamayı da aynı standart kutuphaneyle linklemek gerekir.
option(AIRSIM_USE_LIBCXX "Link against libc++ (required if AirLib was built with clang/libc++)" OFF)

if(AIRSIM_USE_LIBCXX)
    target_compile_options(${PROJECT_NAME} PRIVATE -stdlib=libc++)
    target_link_options(${PROJECT_NAME} PRIVATE -stdlib=libc++)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
