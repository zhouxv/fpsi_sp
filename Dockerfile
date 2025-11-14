FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

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

WORKDIR /root

COPY . /root/FPSI

RUN chmod +x /root/FPSI/build.sh && \
    /root/FPSI/build.sh

RUN mkdir -p /root/FPSI/build && \
    cd /root/FPSI/build && \
    cmake .. && \
    make -j
