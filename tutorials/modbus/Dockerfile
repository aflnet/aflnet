FROM ubuntu:20.04

# Avoid interactive prompts during apt install
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies for AFLNet and Libmodbus
RUN apt-get update && apt-get install -y \
    build-essential \
    clang \
    llvm \
    llvm-dev \
    graphviz-dev \
    libcap-dev \
    git \
    python3 \
    automake \
    autoconf \
    libtool \
    wget \
    make \
    && rm -rf /var/lib/apt/lists/*

# Set up AFLNet environment variables
ENV LLVM_CONFIG="llvm-config"
ENV AFLNET="/opt/aflnet"
ENV PATH="${PATH}:${AFLNET}"
ENV AFL_PATH="${AFLNET}"
ENV AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 \
    AFL_SKIP_CPUFREQ=1 \
    AFL_SKIP_CORE_PATTERN_CHECK=1

# Copy project source to the container
# NOTE: This Dockerfile must be built from the root of the aflnet_modbus repository
COPY . /opt/aflnet
WORKDIR /opt/aflnet

# Build AFLNet
RUN make clean all && \
    cd llvm_mode && \
    make

# --- Tutorial Specific Setup ---

# 1. Compile libmodbus server with instrumentation
WORKDIR /root
RUN git clone https://github.com/stephane/libmodbus.git
WORKDIR /root/libmodbus
# Build libmodbus statically with afl-gcc
RUN ./autogen.sh && \
    CC=afl-gcc ./configure --enable-static --disable-shared && \
    make

# 2. Prepare seeds
WORKDIR /root
RUN mkdir in-modbus
# Generate seeds using the script provided in the tutorial
RUN python3 /opt/aflnet/tutorials/modbus/create_seed.py /root/in-modbus

# 3. Set workdir for fuzzing
WORKDIR /root/libmodbus/tests

# Default command to run the fuzzer
CMD ["/bin/bash"]
# Actual command:
# /opt/aflnet/afl-fuzz -d -i /root/in-modbus -o /root/out-modbus -N tcp://127.0.0.1/1502 -P MODBUS -D 100000 -t 2000 -q 3 -s 3 -E -R ./unit-test-server tcp
