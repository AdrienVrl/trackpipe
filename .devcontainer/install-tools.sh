#!/usr/bin/env bash
# Build and dev tools shared by the dev-gpu and ci-cpu images.
# Both stages are Ubuntu 24.04, so apt package names match.
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends \
    `# C++ toolchain: GCC 13 is the 24.04 default; Clang 18 for clang-tidy/clangd` \
    build-essential gcc g++ clang-18 clang-tidy-18 clang-format-18 clangd-18 lld-18 \
    gdb cmake ninja-build ccache pkg-config git ca-certificates curl sudo \
    `# Libraries for Phases 1-2 (ONNX Runtime comes in Phase 3)` \
    libopencv-dev libgtest-dev libgmock-dev libbenchmark-dev \
    `# Camera, streaming and serial debugging` \
    v4l-utils libv4l-dev ffmpeg picocom \
    `# Python for benchmark and plotting scripts` \
    python3 python3-pip python3-venv

# Unversioned names for the LLVM tools (clang-tidy, clangd, ...)
for tool in clang clang++ clang-tidy clang-format clangd lld; do
    ln -sf "/usr/bin/${tool}-18" "/usr/local/bin/${tool}"
done

CMAKE_VERSION=3.30.5
ARCH=$(uname -m)   # x86_64 now, aarch64 later
curl -fsSL "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-${ARCH}.tar.gz" \
    | tar -xz -C /opt
ln -sf /opt/cmake-${CMAKE_VERSION}-linux-${ARCH}/bin/* /usr/local/bin/

apt-get clean
rm -rf /var/lib/apt/lists/*
