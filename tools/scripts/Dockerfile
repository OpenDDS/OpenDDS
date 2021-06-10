# Using Docker for OpenDDS development
#
# Images created from this Dockerfile can be used to launch containers
# as build platforms for OpenDDS.  The difference between this image
# and thes one created by $DDS_ROOT/Dockerfile is that for this image the source
# tree resides in the Docker host and Docker's "bind mount" feature is used
# to make those files available to the container.
# Since artifacts created during the build are in this same tree, they are also
# available in the host system and persist across container runs.
# Programs that change the version-controlled source code (like git and editors)
# are run from the host, while programs that build and run OpenDDS (like make,
# the compiler, opendds_idl, and gdb) are run in the container.
#
# Start from a clean checkout of OpenDDS with LF line endings
# - On Windows hosts, set git's core.autocrlf to false and core.eol to lf
# - If you already have a git repository, git worktrees are useful for creating
#   a separate checkout on a different branch with (possibly) different config
#
# If OpenDDS tests are enabled, tests/googletest needs to have the googletest
# source code
# - This is often done with recursive clone or submodule update --init
#
# Build a docker image from this Dockerfile (commands are run from $DDS_ROOT)
# - docker build -t opendds-devel -f tools/scripts/Dockerfile .
# Launch the container
# - docker run --rm -ti -v <this_directory>:/opt/OpenDDS opendds-devel
#
# possible extensions:
# - create another Dockerfile that layers on top of this one and builds ACE_TAO
# (outside of /opt/OpenDDS) according to desired configuration

FROM ubuntu:focal

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y \
    cmake \
    curl \
    g++ \
    gdb \
    libssl-dev \
    libxerces-c-dev \
    make \
    ninja-build \
    perl \
    valgrind

WORKDIR /opt/OpenDDS

CMD /bin/bash
