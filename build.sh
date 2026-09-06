#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

: "${COTS_ROOT:=$(cd "${SCRIPT_DIR}/../../cots" && pwd)}"
: "${AIRSIM_ROOT:=${COTS_ROOT}/AirSim}"
: "${BUILD_TYPE:=Release}"

export COTS_ROOT
export AIRSIM_ROOT

echo "COTS_ROOT   = ${COTS_ROOT}"
echo "AIRSIM_ROOT = ${AIRSIM_ROOT}"

cmake -S "${SCRIPT_DIR}" -B "${SCRIPT_DIR}/build" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
cmake --build "${SCRIPT_DIR}/build" -j"$(nproc)"

echo "Binary: ${SCRIPT_DIR}/build/bin/AirSimYolov8Nano"
