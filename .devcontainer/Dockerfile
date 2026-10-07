# trackpipe development images
#
#   ci-cpu  : slim Ubuntu 24.04, CPU-only. Used by CI and later as the base
#             for the ARM64 cross-build.
#   dev-gpu : NVIDIA NGC TensorRT image (CUDA + TensorRT preinstalled) plus
#             the same tools. Used for day-to-day development.
#
# Build one stage explicitly, e.g.:
#   docker build --target ci-cpu -f .devcontainer/Dockerfile .devcontainer

# NGC TensorRT tag. Before changing it, check two things:
#   1. Driver: `nvidia-smi` in WSL shows a "CUDA Version" at least as new as
#      the CUDA version of this container release.
#   2. GPU: the TensorRT version supports Turing (compute capability 7.5).
ARG TRT_TAG=26.02-py3
ARG USERNAME=dev
ARG USER_UID=1000
ARG USER_GID=1000

# ---------------------------------------------------------------------------
# CPU-only image: CI, and later the base for the ARM64 cross-build
# ---------------------------------------------------------------------------
FROM ubuntu:24.04 AS ci-cpu
ARG USERNAME
ARG USER_UID
ARG USER_GID

COPY install-tools.sh /tmp/install-tools.sh
RUN bash /tmp/install-tools.sh && rm /tmp/install-tools.sh

# Ubuntu 24.04 images ship a default "ubuntu" user with UID 1000; remove it
# so our user can take UID 1000 and match your WSL user (file permissions
# on the bind-mounted repo then just work).
RUN (userdel -r ubuntu 2>/dev/null || true) \
 && groupadd --gid ${USER_GID} ${USERNAME} \
 && useradd --uid ${USER_UID} --gid ${USER_GID} -m -s /bin/bash ${USERNAME} \
 && usermod -aG dialout,video ${USERNAME} \
 && echo "${USERNAME} ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
 && chmod 0440 /etc/sudoers.d/${USERNAME}

# Where Claude Code keeps its login and settings (mounted as a volume)
RUN mkdir -p /home/${USERNAME}/.claude && chown ${USERNAME}: /home/${USERNAME}/.claude

ENV CCACHE_DIR=/home/${USERNAME}/.cache/ccache
USER ${USERNAME}
WORKDIR /home/${USERNAME}

# ---------------------------------------------------------------------------
# GPU development image
# ---------------------------------------------------------------------------
FROM nvcr.io/nvidia/tensorrt:${TRT_TAG} AS dev-gpu
ARG USERNAME
ARG USER_UID
ARG USER_GID

COPY install-tools.sh /tmp/install-tools.sh
RUN bash /tmp/install-tools.sh && rm /tmp/install-tools.sh

RUN (userdel -r ubuntu 2>/dev/null || true) \
 && groupadd --gid ${USER_GID} ${USERNAME} \
 && useradd --uid ${USER_UID} --gid ${USER_GID} -m -s /bin/bash ${USERNAME} \
 && usermod -aG dialout,video ${USERNAME} \
 && echo "${USERNAME} ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
 && chmod 0440 /etc/sudoers.d/${USERNAME}

RUN mkdir -p /home/${USERNAME}/.claude && chown ${USERNAME}: /home/${USERNAME}/.claude

# The NGC image's entrypoint prints a banner and driver checks; keep it,
# but make sure CUDA tools are on PATH for login shells too.
ENV PATH=/usr/local/cuda/bin:${PATH}
ENV CCACHE_DIR=/home/${USERNAME}/.cache/ccache

COPY --chmod=755 verify.sh /usr/local/bin/verify-env
USER ${USERNAME}
WORKDIR /home/${USERNAME}
