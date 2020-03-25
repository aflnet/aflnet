FROM ubuntu:16.04

# Install common dependencies
RUN apt-get -y update && \
    apt-get -y install sudo \ 
    apt-utils \
    build-essential \
    openssl \
    clang \
    graphviz-dev \
    git \
    libgnutls-dev

# Add a new user ubuntu, pass: ubuntu
RUN groupadd ubuntu && \
    useradd -rm -d /home/ubuntu -s /bin/bash -g ubuntu -G sudo -u 1000 ubuntu -p "$(openssl passwd -1 ubuntu)"

# Use ubuntu as default username
USER ubuntu
WORKDIR /home/ubuntu

# Download and compile AFLNet
ENV LLVM_CONFIG="llvm-config-3.8"

RUN git clone https://github.com/aflnet/aflnet && \
    cd aflnet && \
    make clean all && \
    cd llvm_mode make && make

# Set up environment variables for AFLNet
ENV AFLNET="/home/ubuntu/aflnet"
ENV PATH="${PATH}:${AFLNET}"
ENV AFL_PATH="${AFLNET}"
ENV AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 \
    AFL_SKIP_CPUFREQ=1

# Download and compile Live555
RUN cd /home/ubuntu && \
    git clone https://github.com/rgaufman/live555.git && \
    cd live555 && \
    git checkout ceeb4f4 && \
    patch -p1 < $AFLNET/tutorials/live555/ceeb4f4_states_decomposed.patch && \
    ./genMakefiles linux && \
    make clean all

# Set up Live555 for fuzzing
RUN cd /home/ubuntu/live555/testProgs && \
    cp ${AFLNET}/tutorials/live555/sample_media_sources/*.* ./
