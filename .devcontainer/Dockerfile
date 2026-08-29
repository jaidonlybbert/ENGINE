FROM ubuntu:24.04
# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

ARG VULKAN_SDK_VERSION=1.4.335.0

# Install core build tools + LLVM/Clang from standard repos
RUN apt-get update && apt-get install -y \
    build-essential \
    ninja-build \
    git \
    libssl-dev \
    libgl-dev \
    libgl1-mesa-dev \
    xorg-dev \
    gcc-14 \
    libwayland-dev \
    build-essential \
    curl \
    wget \
    tar \
    gnupg \
    unzip \
    pkg-config \
    python3 \
    python3-pip \
    software-properties-common \
    clang \
    clang-format \
    clang-tidy \
    clangd \
    libx11-dev \
    libx11-xcb-dev \
    libfontenc-dev \
    libice-dev \
    libsm-dev \
    libxau-dev \
    libxaw7-dev \
    libxcb-glx0-dev \
    libxcb-render0-dev \
    libxcb-render-util0-dev \
    libxcb-xkb-dev \
    libxcb-icccm4-dev \
    libxcb-image0-dev \
    libxcb-keysyms1-dev \
    libxcb-randr0-dev \
    libxcb-shape0-dev \
    libxcb-sync-dev \
    libxcb-xfixes0-dev \
    libxcb-xinerama0-dev \
    libxcb-dri3-dev \
    libxcb-cursor-dev \
    libxcb-dri2-0-dev \
    libxcb-present-dev \
    libxcb-composite0-dev \
    libxcb-ewmh-dev \
    libxcb-res0-dev \
    libxcb-util-dev \
    libxcb-util0-dev

# Install more recent version of CMake from official kitware APT repo than
# is available on base Ubuntu image repos
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc \
    | gpg --dearmor - \
    | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
RUN echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] \
    https://apt.kitware.com/ubuntu/ noble main" \
    | tee /etc/apt/sources.list.d/kitware.list
RUN apt-get update && apt-get install -y cmake=4.4.2-0kitware1ubuntu24.04.1

# Install Conan
RUN pip3 install --break-system-packages conan

# Install the Vulkan SDK
RUN curl -L https://sdk.lunarg.com/sdk/download/${VULKAN_SDK_VERSION}/linux/vulkan-sdk.tar.xz \
    -o /tmp/vulkan-sdk.tar.xz \
 && mkdir -p /opt/vulkan-sdk \
 && tar -xJf /tmp/vulkan-sdk.tar.xz -C /opt/vulkan-sdk --strip-components=1 \
 && rm /tmp/vulkan-sdk.tar.xz

ENV LD_LIBRARY_PATH=""
ENV VULKAN_SDK=/opt/vulkan-sdk
ENV PATH=${VULKAN_SDK}/bin:${PATH}
ENV LD_LIBRARY_PATH=${VULKAN_SDK}/lib:${LD_LIBRARY_PATH}
ENV VK_LAYER_PATH=${VULKAN_SDK}/share/vulkan/explicit_layer.d

# Create workspace directory
RUN mkdir -p /workspace

# Default working directory
WORKDIR /workspace
