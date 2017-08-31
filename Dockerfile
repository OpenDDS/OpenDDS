FROM ubuntu

RUN apt-get update && apt-get install -y \
    curl \
    g++ \
    make \
    perl-base \
    perl-modules

ADD . /opt/OpenDDS

RUN cd /opt/OpenDDS && \
    ./configure --prefix=/usr/local --no-tests && \
    make && \
    make install && \
    cp -a /opt/OpenDDS/ACE_wrappers/MPC /usr/local/share/ace/MPC

ENV ACE_ROOT=/usr/local/share/ace \
    TAO_ROOT=/usr/local/share/tao \
    DDS_ROOT=/usr/local/share/dds \
    PATH=".:/usr/local/share/ace/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

WORKDIR /opt/workspace
