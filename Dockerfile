FROM ubuntu:xenial

RUN apt-get update && apt-get install -y \
    curl \
    g++ \
    make \
    libxerces-c-dev \
    libssl-dev \
    perl-base \
    perl-modules

ADD . /opt/OpenDDS

RUN rm -rf /opt/OpenDDS/ACE_wrappers

RUN cd /opt/OpenDDS && \
    ./configure --prefix=/usr/local --doc_group --std=c++11 --no-tests --security && \
    make && \
    make install && \
    cp -a /opt/OpenDDS/ACE_wrappers/MPC /usr/local/share/ace/MPC

ENV ACE_ROOT=/usr/local/share/ace \
    TAO_ROOT=/usr/local/share/tao \
    DDS_ROOT=/usr/local/share/dds \
    PATH=".:/usr/local/share/ace/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

WORKDIR /opt/workspace
