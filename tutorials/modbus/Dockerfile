# Build from the root of the AFLNet repository:
#   docker build -f tutorials/modbus/Dockerfile -t aflnet-modbus .
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get -y update && \
    apt-get -y install \
    build-essential \
    graphviz-dev \
    libcap-dev \
    git \
    autoconf \
    automake \
    libtool \
    && rm -rf /var/lib/apt/lists/*

# Build AFLNet from this checkout (afl-gcc is enough, llvm_mode is not needed)
COPY . /opt/aflnet
WORKDIR /opt/aflnet
RUN make clean all

ENV AFLNET="/opt/aflnet"
ENV PATH="${PATH}:${AFLNET}"
ENV AFL_PATH="${AFLNET}"
ENV AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 \
    AFL_SKIP_CPUFREQ=1

# Build libmodbus and its unit-test-server, instrumented with afl-gcc
WORKDIR /opt
RUN git clone https://github.com/stephane/libmodbus.git && \
    cd libmodbus && \
    git checkout v3.2.0 && \
    ./autogen.sh && \
    CC=afl-gcc ./configure --enable-static --disable-shared && \
    make

WORKDIR /opt/libmodbus/tests
CMD ["/bin/bash"]
