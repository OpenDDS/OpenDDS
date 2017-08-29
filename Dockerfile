FROM ubuntu

RUN apt-get update && apt-get install -y perl-base perl-modules g++ make libwww-perl curl wget
ADD . /opt/OpenDDS
WORKDIR /opt/OpenDDS
RUN ./configure --prefix=/usr/local --no-tests
RUN make
RUN make install
RUN cp -a /opt/OpenDDS/ACE_wrappers/MPC /usr/local/share/ace/MPC
ENV ACE_ROOT /usr/local/share/ace
ENV TAO_ROOT /usr/local/share/tao
ENV DDS_ROOT /usr/local/share/dds
ENV PATH ".:/usr/local/share/ace/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"

WORKDIR /opt/workspace
