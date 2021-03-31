FROM ubuntu:18.04

# Install common dependencies
RUN apt-get -y update && \
    apt-get -y install sudo \
    build-essential \
    autoconf \
    avahi-daemon \
    avahi-utils \
    cura-engine \
    libavahi-client-dev \
    libfreetype6-dev \
    libgnutls28-dev \
    libharfbuzz-dev \
    libjbig2dec0-dev \
    libjpeg-dev \
    libmupdf-dev \
    libnss-mdns \
    libopenjp2-7-dev \
    libpng-dev \
    zlib1g-dev \
    net-tools \
    iputils-ping \
    vim \
    avahi-daemon \
    tcpdump \
    man \
    curl \
    openssl \
    clang \
    graphviz-dev \
    git \
    libgnutls28-dev \
    python-pip \
    nano \
    net-tools \
    gdb

# Make changes necessary to run bonjour
RUN sed -ie 's/rlimit-nproc=3/rlimit-nproc=8/' /etc/avahi/avahi-daemon.conf
RUN update-rc.d dbus defaults
RUN update-rc.d avahi-daemon defaults

# Create entrypoint.sh script to start dbus and avahi-daemon
RUN echo '#!/bin/bash\n\
sudo service dbus start\n\
sudo service avahi-daemon start\n\
mkdir /tmp/spool\n\
$*\n\
' > /usr/bin/entrypoint.sh && chmod +x /usr/bin/entrypoint.sh
ENTRYPOINT ["bash", "/usr/bin/entrypoint.sh"]

# Add a new user ubuntu, pass: ubuntu
RUN groupadd ubuntu && \
    useradd -rm -d /home/ubuntu -s /bin/bash -g ubuntu -G sudo -u 1000 ubuntu -p "$(openssl passwd -1 ubuntu)"

RUN echo "ALL ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

# Use ubuntu as default username
USER ubuntu
WORKDIR /home/ubuntu

# Download and compile AFLNet
ENV LLVM_CONFIG="llvm-config-6.0"

# Set up fuzzers
RUN git clone https://github.com/harrypale/aflnet.git && \
    cd aflnet && \
    make clean all && \
    cd llvm_mode && make

# Set up environment variables for AFLNet
ENV WORKDIR="/home/ubuntu/experiments"
ENV AFLNET="/home/ubuntu/aflnet"
ENV PATH="${PATH}:${AFLNET}:/home/ubuntu/.local/bin:${WORKDIR}"
ENV AFL_PATH="${AFLNET}"

ENV AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1 \
    AFL_SKIP_CPUFREQ=1 \
    AFL_NO_AFFINITY=1

RUN mkdir $WORKDIR && \
    pip install gcovr

COPY --chown=ubuntu:ubuntu ipp.dict ${WORKDIR}/ipp.dict
COPY --chown=ubuntu:ubuntu in-ipp ${WORKDIR}/in-ipp
COPY --chown=ubuntu:ubuntu ippcleanup.sh ${WORKDIR}/ippcleanup.sh

# Download and compile ippsample
RUN cd $WORKDIR && \
    git clone https://github.com/istopwg/ippsample.git ippsample && \
    cd ippsample && \
    git checkout 1ee7bcd4d0ed0e1e49b434c0ab296bb0c9499c0d && \
    CC=$AFLNET/afl-clang ./configure && \
    make clean all
# RUN sudo make install
