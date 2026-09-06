# FindAirSim.cmake
#
# AIRSIM_ROOT (cache/env) altındaki AirSim kurulumunu bulur ve
# AirSim::AirLib imported target'ını olusturur.
#
# Aranan yapı (AirSim build.sh sonrası):
#   ${AIRSIM_ROOT}/AirLib/include
#   ${AIRSIM_ROOT}/AirLib/deps/eigen3
#   ${AIRSIM_ROOT}/AirLib/deps/rpclib/include
#   ${AIRSIM_ROOT}/AirLib/deps/MavLinkCom/include
#   ${AIRSIM_ROOT}/AirLib/lib/...  (libAirLib.a, librpc.a, libMavLinkCom.a)

if(NOT AIRSIM_ROOT)
    if(DEFINED ENV{AIRSIM_ROOT})
        set(AIRSIM_ROOT "$ENV{AIRSIM_ROOT}" CACHE PATH "AirSim root directory")
    endif()
endif()

if(NOT AIRSIM_ROOT)
    message(FATAL_ERROR "AIRSIM_ROOT is not set. Export it or pass -DAIRSIM_ROOT=<path>.")
endif()

# ---- include dizinleri -------------------------------------------------------

find_path(AIRSIM_AIRLIB_INCLUDE_DIR
    NAMES vehicles/multirotor/api/MultirotorRpcLibClient.hpp
    HINTS "${AIRSIM_ROOT}/AirLib/include"
    NO_DEFAULT_PATH)

find_path(AIRSIM_EIGEN_INCLUDE_DIR
    NAMES Eigen/Dense
    HINTS "${AIRSIM_ROOT}/AirLib/deps/eigen3"
          "${AIRSIM_ROOT}/external/eigen3"
    NO_DEFAULT_PATH)

find_path(AIRSIM_RPCLIB_INCLUDE_DIR
    NAMES rpc/client.h
    HINTS "${AIRSIM_ROOT}/AirLib/deps/rpclib/include"
          "${AIRSIM_ROOT}/external/rpclib/rpclib-2.3.0/include"
    NO_DEFAULT_PATH)

find_path(AIRSIM_MAVLINK_INCLUDE_DIR
    NAMES MavLinkConnection.hpp
    HINTS "${AIRSIM_ROOT}/AirLib/deps/MavLinkCom/include"
          "${AIRSIM_ROOT}/MavLinkCom/include"
    NO_DEFAULT_PATH)

# ---- kutuphaneler ------------------------------------------------------------

set(AIRSIM_LIB_HINTS
    "${AIRSIM_ROOT}/AirLib/lib"
    "${AIRSIM_ROOT}/AirLib/lib/x64/Release"
    "${AIRSIM_ROOT}/AirLib/lib/x64/Debug"
    "${AIRSIM_ROOT}/AirLib/deps/rpclib/lib"
    "${AIRSIM_ROOT}/AirLib/deps/rpclib/lib/x64/Release"
    "${AIRSIM_ROOT}/AirLib/deps/rpclib/lib/x64/Debug"
    "${AIRSIM_ROOT}/AirLib/deps/MavLinkCom/lib"
    "${AIRSIM_ROOT}/MavLinkCom/lib"
    "${AIRSIM_ROOT}/build_release/output/lib"
    "${AIRSIM_ROOT}/build_debug/output/lib")

find_library(AIRSIM_AIRLIB_LIBRARY   NAMES AirLib      HINTS ${AIRSIM_LIB_HINTS} NO_DEFAULT_PATH)
find_library(AIRSIM_RPC_LIBRARY      NAMES rpc         HINTS ${AIRSIM_LIB_HINTS} NO_DEFAULT_PATH)
find_library(AIRSIM_MAVLINK_LIBRARY  NAMES MavLinkCom  HINTS ${AIRSIM_LIB_HINTS} NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AirSim
    REQUIRED_VARS
        AIRSIM_AIRLIB_INCLUDE_DIR
        AIRSIM_EIGEN_INCLUDE_DIR
        AIRSIM_RPCLIB_INCLUDE_DIR
        AIRSIM_AIRLIB_LIBRARY
        AIRSIM_RPC_LIBRARY)

if(AirSim_FOUND AND NOT TARGET AirSim::AirLib)
    find_package(Threads REQUIRED)

    add_library(AirSim::AirLib UNKNOWN IMPORTED)

    set_target_properties(AirSim::AirLib PROPERTIES
        IMPORTED_LOCATION "${AIRSIM_AIRLIB_LIBRARY}"
        INTERFACE_COMPILE_DEFINITIONS "RPCLIB_MSGPACK=clmdep_msgpack"
        INTERFACE_INCLUDE_DIRECTORIES
            "${AIRSIM_AIRLIB_INCLUDE_DIR};${AIRSIM_EIGEN_INCLUDE_DIR};${AIRSIM_RPCLIB_INCLUDE_DIR}")

    set(AIRSIM_LINK_LIBRARIES "${AIRSIM_RPC_LIBRARY}")

    if(AIRSIM_MAVLINK_LIBRARY)
        list(APPEND AIRSIM_LINK_LIBRARIES "${AIRSIM_MAVLINK_LIBRARY}")
    endif()

    list(APPEND AIRSIM_LINK_LIBRARIES Threads::Threads)

    set_target_properties(AirSim::AirLib PROPERTIES
        INTERFACE_LINK_LIBRARIES "${AIRSIM_LINK_LIBRARIES}")

    if(AIRSIM_MAVLINK_INCLUDE_DIR)
        set_property(TARGET AirSim::AirLib APPEND PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES "${AIRSIM_MAVLINK_INCLUDE_DIR}")
    endif()
endif()

mark_as_advanced(
    AIRSIM_AIRLIB_INCLUDE_DIR
    AIRSIM_EIGEN_INCLUDE_DIR
    AIRSIM_RPCLIB_INCLUDE_DIR
    AIRSIM_MAVLINK_INCLUDE_DIR
    AIRSIM_AIRLIB_LIBRARY
    AIRSIM_RPC_LIBRARY
    AIRSIM_MAVLINK_LIBRARY)
