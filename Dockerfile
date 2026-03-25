FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        git \
        build-essential \
        clang \
        clang-18 \
        llvm-18-dev \
        llvm-18-tools \
        lld-18 \
        openssl \
        graphviz \
        libgraphviz-dev \
        libcap-dev \
    && rm -rf /var/lib/apt/lists/*

# Toolchain default for LLVM mode (passed explicitly to llvm_mode build below)
ENV LLVM_CONFIG=llvm-config-18

# Download and compile AFLNet
RUN git clone --depth 1 https://github.com/Rosayxy/aflnet.git /opt/aflnet
WORKDIR /opt/aflnet

RUN make clean all \
    && make -C llvm_mode clean all CC=clang-18 CXX=clang++-18 LLVM_CONFIG=llvm-config-18

# Set up environment variables for AFLNet
ENV AFLNET="/opt/aflnet"
ENV PATH="${PATH}:${AFLNET}"
ENV AFL_PATH="${AFLNET}"
ENV AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 \
    AFL_SKIP_CPUFREQ=1
