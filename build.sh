#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

IMAGE_NAME="${IMAGE_NAME:-dockir-builder}"
BUILD_DIR="${BUILD_DIR:-build}"
CLEAN_BUILD=0
MODE="auto"

usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Build the DockIR firmware (pico_ir_keyboard.uf2).

Options:
  --docker      Build using Docker (default when Docker is available on host)
  --local       Build directly on the current machine using local toolchain
  --clean       Remove the build directory before building
  -h, --help    Show this help message

Environment Variables:
  IMAGE_NAME    Docker image tag to use (default: dockir-builder)
  BUILD_DIR     Build output directory (default: build)
  PICO_SDK_PATH Path to Raspberry Pi Pico SDK (for --local mode)
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --docker)
            MODE="docker"
            shift
            ;;
        --local)
            MODE="local"
            shift
            ;;
        --clean)
            CLEAN_BUILD=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Error: Unknown option '$1'" >&2
            usage >&2
            exit 1
            ;;
    esac
done

build_local() {
    if [[ "${CLEAN_BUILD}" -eq 1 ]]; then
        echo "==> Cleaning ${BUILD_DIR}..."
        rm -rf "${BUILD_DIR}"
    fi

    if [[ -z "${PICO_SDK_PATH:-}" ]]; then
        if [[ -d "/opt/pico-sdk" ]]; then
            export PICO_SDK_PATH="/opt/pico-sdk"
        else
            echo "Error: PICO_SDK_PATH is not set and /opt/pico-sdk was not found." >&2
            echo "Set PICO_SDK_PATH or run with Docker: ./build.sh --docker" >&2
            exit 1
        fi
    fi

    echo "==> Using PICO_SDK_PATH=${PICO_SDK_PATH}"
    mkdir -p "${BUILD_DIR}"

    cmake -S . -B "${BUILD_DIR}" -DPICO_SDK_PATH="${PICO_SDK_PATH}"

    JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || nproc 2>/dev/null || echo 4)"
    cmake --build "${BUILD_DIR}" --parallel "${JOBS}"

    UF2_FILE="${BUILD_DIR}/pico_ir_keyboard.uf2"
    if [[ -f "${UF2_FILE}" ]]; then
        cp -f "${UF2_FILE}" "${SCRIPT_DIR}/pico_ir_keyboard.uf2"
        echo "==> Build succeeded!"
        echo "    UF2 file: ${SCRIPT_DIR}/${UF2_FILE}"
        echo "    Copied:   ${SCRIPT_DIR}/pico_ir_keyboard.uf2"
    else
        echo "Error: Build completed, but ${UF2_FILE} was not found." >&2
        exit 1
    fi
}

build_docker() {
    if ! command -v docker >/dev/null 2>&1; then
        echo "Error: docker command not found. Install Docker or run './build.sh --local'." >&2
        exit 1
    fi

    echo "==> Building Docker image '${IMAGE_NAME}'..."
    docker build -t "${IMAGE_NAME}" "${SCRIPT_DIR}"

    LOCAL_ARGS=("--local")
    if [[ "${CLEAN_BUILD}" -eq 1 ]]; then
        LOCAL_ARGS+=("--clean")
    fi

    echo "==> Running firmware build in container..."
    docker run --rm \
        -u "$(id -u):$(id -g)" \
        -e BUILD_DIR="${BUILD_DIR}" \
        -v "${SCRIPT_DIR}:/workspace" \
        -w /workspace \
        "${IMAGE_NAME}" \
        ./build.sh "${LOCAL_ARGS[@]}"
}

if [[ "${MODE}" == "auto" ]]; then
    if [[ -f "/.dockerenv" ]] || [[ -n "${PICO_SDK_PATH:-}" && -x "$(command -v arm-none-eabi-gcc 2>/dev/null || true)" ]]; then
        MODE="local"
    else
        MODE="docker"
    fi
fi

if [[ "${MODE}" == "local" ]]; then
    build_local
else
    build_docker
fi
