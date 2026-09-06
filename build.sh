#!/usr/bin/env bash
set -euo pipefail

# Kullanım:
#   ./build.sh            -> ortama gore otomatik (AirSim varsa server, CUDA varsa client)
#   ./build.sh server     -> sadece server
#   ./build.sh client     -> sadece client

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET="${1:-auto}"

: "${COTS_ROOT:=$(cd "${SCRIPT_DIR}/../../cots" && pwd)}"
: "${AIRSIM_ROOT:=${COTS_ROOT}/AirSim}"
: "${CUDA_ROOT:=/usr/local/cuda}"
: "${BUILD_TYPE:=Release}"

export COTS_ROOT AIRSIM_ROOT CUDA_ROOT

CMAKE_ARGS=(-DCMAKE_BUILD_TYPE="${BUILD_TYPE}")

if [ "${TARGET}" = "server" ]; then
    CMAKE_ARGS+=(-DBUILD_SERVER=ON -DBUILD_CLIENT=OFF)
elif [ "${TARGET}" = "client" ]; then
    CMAKE_ARGS+=(-DBUILD_SERVER=OFF -DBUILD_CLIENT=ON)
fi

echo "COTS_ROOT   = ${COTS_ROOT}"
echo "AIRSIM_ROOT = ${AIRSIM_ROOT}"
echo "CUDA_ROOT   = ${CUDA_ROOT}"
echo "TARGET      = ${TARGET}"

cmake -S "${SCRIPT_DIR}" -B "${SCRIPT_DIR}/build" "${CMAKE_ARGS[@]}"
cmake --build "${SCRIPT_DIR}/build" -j"$(nproc)"

echo "Output: ${SCRIPT_DIR}/build/bin/"
ls -1 "${SCRIPT_DIR}/build/bin/" 2>/dev/null || true
