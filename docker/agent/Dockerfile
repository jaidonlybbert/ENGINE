ARG UBUNTU_VERSION=24.04

FROM ubuntu:${UBUNTU_VERSION}
# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

ARG VULKAN_SDK_VERSION=1.4.335.0

# Same native build toolchain as .devcontainer/Dockerfile (issue #64's agent image is kept
# self-contained rather than FROM'd off that one, to avoid a fragile cross-Dockerfile build
# dependency - update both if the toolchain changes).
RUN apt-get update && apt-get install -y \
    build-essential \
    ninja-build \
    git \
    libssl-dev \
    libgl-dev \
    libgl1-mesa-dev \
    xorg-dev \
    gcc-14 \
    g++-14 \
    libwayland-dev \
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
    ca-certificates \
    gdb \
    valgrind \
    graphviz \
    ccache \
    python3-venv \
    python-is-python3 \
    xvfb \
    mesa-vulkan-drivers \
    vulkan-tools \
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
    libxcb-util0-dev \
    && rm -rf /var/lib/apt/lists/*

# Install a more recent version of CMake from the official Kitware APT repo than is
# available on the base Ubuntu image repos
RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc \
    | gpg --dearmor - \
    | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
RUN echo "deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] \
    https://apt.kitware.com/ubuntu/ noble main" \
    | tee /etc/apt/sources.list.d/kitware.list
RUN apt-get update && apt-get install -y cmake=4.4.2-0kitware1ubuntu24.04.1 \
    && rm -rf /var/lib/apt/lists/*

# Install Conan, and clang-format 20 from pip - CI's style check (.github/workflows/
# style.yml) uses clang-format-20, newer than the version Ubuntu 24.04's apt ships, and
# /usr/local/bin (where pip puts it) comes before /usr/bin on PATH so it wins.
RUN pip3 install --break-system-packages conan clang-format==20.1.3

# Install the Vulkan SDK
RUN curl -L https://sdk.lunarg.com/sdk/download/${VULKAN_SDK_VERSION}/linux/vulkan-sdk.tar.xz \
    -o /tmp/vulkan-sdk.tar.xz \
 && mkdir -p /opt/vulkan-sdk \
 && tar -xJf /tmp/vulkan-sdk.tar.xz -C /opt/vulkan-sdk --strip-components=1 \
 && rm /tmp/vulkan-sdk.tar.xz

ENV VULKAN_SDK=/opt/vulkan-sdk
ENV PATH=${VULKAN_SDK}/x86_64/bin:${PATH}
ENV LD_LIBRARY_PATH=${VULKAN_SDK}/x86_64/lib
ENV VK_LAYER_PATH=${VULKAN_SDK}/x86_64/share/vulkan/explicit_layer.d
ENV Vulkan_LIBRARY=${VULKAN_SDK}/x86_64/lib/libvulkan.so
ENV Vulkan_INCLUDE_DIR=${VULKAN_SDK}/x86_64/include

# Node.js (for the Claude Code CLI, @anthropic-ai/claude-code on npm) and the GitHub CLI
# (so agents can open PRs - see docker/README.md on scoping GITHUB_TOKEN. Deliberately no
# SSH keys anywhere in this image - gh's HTTPS+token credential helper is the only way
# this container can push, so there's never a host SSH private key to leak).
RUN curl -fsSL https://deb.nodesource.com/setup_22.x | bash - \
 && apt-get install -y nodejs \
 && curl -fsSL https://cli.github.com/packages/githubcli-archive-keyring.gpg \
    | gpg --dearmor -o /usr/share/keyrings/githubcli-archive-keyring.gpg \
 && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/githubcli-archive-keyring.gpg] https://cli.github.com/packages stable main" \
    | tee /etc/apt/sources.list.d/github-cli.list \
 && apt-get update && apt-get install -y gh \
 && rm -rf /var/lib/apt/lists/*

RUN npm install -g @anthropic-ai/claude-code

# Non-root user - the agent never needs (and must never have) root inside the container.
# Recent Ubuntu base images already ship a default "ubuntu" user/group at 1000:1000, so
# create ours only if that UID/GID is actually free; otherwise rename the existing one -
# either way we end up with exactly one non-root "agent" user at AGENT_UID:AGENT_GID.
ARG AGENT_UID=1000
ARG AGENT_GID=1000
RUN if getent group ${AGENT_GID} >/dev/null; then \
        groupmod -n agent "$(getent group ${AGENT_GID} | cut -d: -f1)"; \
    else \
        groupadd -g ${AGENT_GID} agent; \
    fi \
 && if getent passwd ${AGENT_UID} >/dev/null; then \
        usermod -l agent -d /home/agent -m -g ${AGENT_GID} "$(getent passwd ${AGENT_UID} | cut -d: -f1)"; \
    else \
        useradd -m -u ${AGENT_UID} -g ${AGENT_GID} -s /bin/bash agent; \
    fi \
 && mkdir -p /workspace /shared \
 && chown -R agent:agent /workspace /shared

COPY --chown=agent:agent entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

USER agent
WORKDIR /workspace

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["bash"]
