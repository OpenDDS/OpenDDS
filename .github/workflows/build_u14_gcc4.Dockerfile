FROM ubuntu:14.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    curl \
    g++-4.4 \
    make \
    libxerces-c-dev \
    libssl-dev \
    perl

ADD . /opt/OpenDDS

RUN cd /opt && \
    curl -LO https://github.com/Kitware/CMake/releases/download/v3.22.1/cmake-3.22.1-linux-x86_64.tar.gz && \
    tar xzf cmake-3.22.1-linux-x86_64.tar.gz

RUN cd /opt/OpenDDS && \
    ./configure --compiler 'g++-4.4' --security --tests --no-rapidjson --cmake=/opt/cmake-3.22.1-linux-x86_64/bin/cmake && \
    make -j $(($(nproc)+1))
