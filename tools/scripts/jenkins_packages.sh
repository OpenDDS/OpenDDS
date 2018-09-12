#!/bin/sh
# Install dependency packages for use by Jenkins (debian stable)
apt-get -y --fix-missing install \
    qtbase5-dev \
    libwireshark-dev \
    libxerces-c-dev \
    libssl-dev \
    openjdk-8-jdk-headless \
    cmake \
    pkg-config \

# blank line ends the list since each entry ends in "\"
