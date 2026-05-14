FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /home

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    openssl \
    build-essential \
    cmake \
    git \
    libtool \
    autoconf \
    automake \
    pkg-config \
    iproute2 \
    python3 \
    sudo \
    nasm \
    libssl-dev \
    libgmp-dev \
    wget \
    libfmt-dev \
    && update-ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY ./bench1.sh\
    ./bench2.sh\
    ./build.sh\
    ./CMakeLists.txt\
    ./throttle.sh\
    /home/FPSI/

RUN cd /home/FPSI && \
    chmod +x build.sh && \
    ./build.sh

COPY ./include/ /home/FPSI/include/
COPY ./src/ /home/FPSI/src/

RUN mkdir -p /home/FPSI/build && \
    cd /home/FPSI/build && \
    cmake .. && \
    make -j

WORKDIR /home/FPSI