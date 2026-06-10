# Yuzu dev container — Ubuntu 24.04 LTS + LLVM 20 + zsh/starship.
FROM ubuntu:24.04

ARG LLVM_VERSION=22
ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=${USER_UID}
ARG DEBIAN_FRONTEND=noninteractive

# Base toolchain. lsb-release/gnupg/wget are needed by the apt.llvm.org
# installer script.
RUN apt-get update && apt-get install -y --no-install-recommends \
  build-essential \
  ca-certificates \
  cmake \
  curl \
  git \
  gnupg \
  less \
  libgmock-dev \
  libgtest-dev \
  libprotobuf-dev \
  libzstd-dev \
  locales \
  lsb-release \
  ninja-build \
  pkg-config \
  protobuf-compiler \
  python3-dev \
  python3-pip \
  software-properties-common \
  sudo \
  unzip \
  wget \
  zlib1g-dev \
  zsh \
  && locale-gen en_US.UTF-8 \
  && rm -rf /var/lib/apt/lists/* \
  && pip3 install --break-system-packages --no-cache-dir lit nanobind duckdb

# LLVM/MLIR from apt.llvm.org. We avoid `llvm.sh ... all` because it pulls
# in libc++/libc++abi dev headers, which drop a `cxxabi.h` into
# /usr/lib/llvm-22/include/ that conflicts with libstdc++'s when the
# project (built with libstdc++) adds LLVM_INCLUDE_DIRS as -isystem.
RUN curl -fsSL https://apt.llvm.org/llvm.sh -o /tmp/llvm.sh \
  && bash /tmp/llvm.sh ${LLVM_VERSION} \
  && rm /tmp/llvm.sh \
  && apt-get update && apt-get install -y --no-install-recommends \
  clang-tools-${LLVM_VERSION} \
  clang-tidy-${LLVM_VERSION} \
  clang-format-${LLVM_VERSION} \
  clangd-${LLVM_VERSION} \
  lld-${LLVM_VERSION} \
  lldb-${LLVM_VERSION} \
  llvm-${LLVM_VERSION} \
  llvm-${LLVM_VERSION}-tools \
  libmlir-${LLVM_VERSION}-dev \
  mlir-${LLVM_VERSION}-tools \
  && rm -rf /var/lib/apt/lists/*

# Point unversioned tool names at the versioned binaries so the existing
# Makefile (which invokes plain `clang`/`clang++`) Just Works.
RUN for tool in clang clang++ clang-tidy clang-format clangd \
  lldb lldb-dap lldb-server \
  llvm-config FileCheck not lld ld.lld; do \
  if [ -e /usr/bin/${tool}-${LLVM_VERSION} ]; then \
  ln -sf /usr/bin/${tool}-${LLVM_VERSION} /usr/local/bin/${tool}; \
  fi; \
  done

# Starship prompt (matches the host setup).
RUN curl -fsSL https://starship.rs/install.sh | sh -s -- -y

# Ubuntu 24.04 ships with a pre-existing UID 1000 `ubuntu` user — drop it so
# our dev user can claim that UID and match bind-mount permissions.
RUN if getent passwd ubuntu >/dev/null; then userdel -r ubuntu 2>/dev/null || true; fi \
  && groupadd --gid ${USER_GID} ${USERNAME} \
  && useradd --uid ${USER_UID} --gid ${USER_GID} -m -s /bin/zsh ${USERNAME} \
  && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
  && chmod 0440 /etc/sudoers.d/${USERNAME}

USER ${USERNAME}

# Oh-my-zsh — unattended so it doesn't drop into an interactive shell.
RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended

COPY --chown=${USERNAME}:${USERNAME} zshrc /home/${USERNAME}/.zshrc
COPY --chown=${USERNAME}:${USERNAME} starship.toml /home/${USERNAME}/.config/starship.toml

ENV LLVM_DIR=/usr/lib/llvm-${LLVM_VERSION}/lib/cmake/llvm \
  MLIR_DIR=/usr/lib/llvm-${LLVM_VERSION}/lib/cmake/mlir \
  LANG=en_US.UTF-8 \
  LC_ALL=en_US.UTF-8

WORKDIR /workspaces/Yuzu
