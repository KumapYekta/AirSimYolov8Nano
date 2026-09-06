cmake_minimum_required(VERSION 3.18)

project(AirSimYolov8Nano
    VERSION 0.3.0
    DESCRIPTION "AirSim -> GStreamer UDP -> Jetson CUDA pipeline"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# -----------------------------------------------------------------------------
# Kok dizinler - once environment variable, yoksa gorece yol
#
#   <workspace>/
#     ├── cots/                 -> 3. parti bagımlılıklar (AirSim, ...)
#     └── projects/
#           └── AirSimYolov8Nano/
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

# -----------------------------------------------------------------------------
# GStreamer - her iki taraf da kullanıyor, bir kez bulunur
# -----------------------------------------------------------------------------

if(DEFINED ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64})
    # Windows ters eğik çizgilerini (C:\...) CMake formatına (C:/...) çevir
    file(TO_CMAKE_PATH "$ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}" _GST_DIR)

    message(STATUS "GSTREAMER_1_0_ROOT_MSVC_X86_64 : ${_GST_DIR}")

    # 1. GStreamer içindeki pkg-config.exe'yi CMake'e göster
    list(PREPEND CMAKE_PROGRAM_PATH "${_GST_DIR}/bin")

    # 2. .pc dosyalarının yolunu ekle (Windows için noktalı virgül ';' kullanılır)
    if(DEFINED ENV{PKG_CONFIG_PATH})
        set(ENV{PKG_CONFIG_PATH} "${_GST_DIR}/lib/pkgconfig;$ENV{PKG_CONFIG_PATH}")
    else()
        set(ENV{PKG_CONFIG_PATH} "${_GST_DIR}/lib/pkgconfig")
    endif()
else()
    message(FATAL_ERROR "GSTREAMER_1_0_ROOT_MSVC_X86_64 ortam degiskeni bulunamadi! GStreamer devel paketinin kurulu oldugundan emin olun.")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET
    gstreamer-1.0>=1.16
    gstreamer-app-1.0
    gstreamer-video-1.0)

# -----------------------------------------------------------------------------
# Hangi taraf derlenecek?
# Server AirSim + VAAPI ister, client CUDA ister. Ikisi farklı makinede
# derlendigi icin varsayılanlar ortama gore otomatik belirleniyor.
# -----------------------------------------------------------------------------

if(EXISTS "${AIRSIM_ROOT}/AirLib/include")
    set(BUILD_SERVER_DEFAULT ON)
else()
    set(BUILD_SERVER_DEFAULT OFF)
endif()

if(DEFINED ENV{CUDA_ROOT})
    set(CUDAToolkit_ROOT "$ENV{CUDA_ROOT}" CACHE PATH "CUDA toolkit root")
elseif(DEFINED ENV{CUDA_HOME})
    set(CUDAToolkit_ROOT "$ENV{CUDA_HOME}" CACHE PATH "CUDA toolkit root")
endif()

find_package(CUDAToolkit QUIET)

if(CUDAToolkit_FOUND)
    set(BUILD_CLIENT_DEFAULT ON)
else()
    set(BUILD_CLIENT_DEFAULT OFF)
endif()

option(BUILD_SERVER "Build the AirSim capture / UDP sender (needs AirSim)" ${BUILD_SERVER_DEFAULT})
option(BUILD_CLIENT "Build the Jetson UDP receiver (needs CUDA)"           ${BUILD_CLIENT_DEFAULT})

message(STATUS "BUILD_SERVER : ${BUILD_SERVER}")
message(STATUS "BUILD_CLIENT : ${BUILD_CLIENT}")

if((NOT BUILD_SERVER) AND (NOT BUILD_CLIENT))
    message(FATAL_ERROR "Neither server nor client can be built. Check AIRSIM_ROOT / CUDA_ROOT.")
endif()

# -----------------------------------------------------------------------------
# Alt projeler
# -----------------------------------------------------------------------------

add_subdirectory(common)

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()
