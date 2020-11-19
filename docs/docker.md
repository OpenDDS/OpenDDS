# Quick Start with Docker

Docker images containing a pre-built OpenDDS are available on
[DockerHub](https://hub.docker.com/r/objectcomputing/opendds/).  An image
corresponding to a particular release has a tag of the form `release-DDS-X.xx`,
e.g., `release-DDS-3.12`.

1. Check for prerequisites

        docker --version
        docker-compose --version

2. Enter a container

        docker run --rm -ti -v "$PWD:/opt/workspace" objectcomputing/opendds

3. Copy the `Messenger` directory which contains an example from the [Developer's Guide](http://download.objectcomputing.com/OpenDDS/OpenDDS-latest.pdf)

        cp -R /opt/OpenDDS/DevGuideExamples/DCPS/Messenger Messenger
        cd Messenger

4. Configure and build the Messenger example

        mwc.pl -type gnuace
        make

5. Exit the container

        exit

6. Enter the `Messenger` directory

        cd Messenger

7. Create an `rtps.ini` file to control discovery with the following content

        [common]
        DCPSGlobalTransportConfig=$file
        DCPSDefaultDiscovery=DEFAULT_RTPS

        [transport/the_rtps_transport]
        transport_type=rtps_udp

8. Run the Messenger example with RTPS

        docker-compose up

9. Run the Messenger example with InfoRepo

        docker-compose -f docker-compose-inforepo.yml up
        # Use Control-C to kill the InfoRepo process
