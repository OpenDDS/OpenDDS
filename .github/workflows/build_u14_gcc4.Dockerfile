FROM ubuntu:14.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    cmake \
    curl \
    g++-4.4 \
    make \
    libxerces-c-dev \
    libssl-dev \
    perl-base \
    perl-modules \
    git

ADD . /opt/OpenDDS

RUN cd /opt/OpenDDS && \
    ./configure --compiler 'g++-4.4' --security --tests && \
    make -j $(($(nproc)+1))
