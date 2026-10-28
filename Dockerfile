FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies for Raspberry Pi Pico SDK
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    gcc-arm-none-eabi \
    git \
    libnewlib-arm-none-eabi \
    libstdc++-arm-none-eabi-newlib \
    ninja-build \
    pkg-config \
    python3 \
    && rm -rf /var/lib/apt/lists/*

# Clone Raspberry Pi Pico SDK (1.5.1 includes elf2uf2 and TinyUSB family_configure_device_example support)
ARG PICO_SDK_VERSION=1.5.1
ENV PICO_SDK_PATH=/opt/pico-sdk

RUN git clone --branch "${PICO_SDK_VERSION}" --depth 1 \
        https://github.com/raspberrypi/pico-sdk.git "${PICO_SDK_PATH}" && \
    cd "${PICO_SDK_PATH}" && \
    git submodule update --init --depth 1

WORKDIR /workspace

CMD ["./build.sh", "--local"]
