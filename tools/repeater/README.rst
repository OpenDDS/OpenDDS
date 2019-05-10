===========================
Multicast Datagram Repeater
===========================

The multicast datagram repeater allows existing multicast-dependent programs
to be used (without configuration or code changes) on networks that don't route
multicast traffic among all hosts.

The multicast datagram repeater takes datagrams that are received on a
multicast group and forwards them to any number of unicast addresses.
Concurrently, datagrams received on a known port (which we'll call the
unicast port) are forwarded to the same multicast group.

Note that this isn't compatible with protocols that make use of the source IP
address and/or port when receiving multicast datagrams (for example, to use as a
"return address"), since the address bound to this process's socket will appear
to be the source address instead of the address of the originating socket.
OpenDDS's implementation of the OMG RTPS Simple Participant Discovery Protocol
(SPDP) doesn't use the source address.

Basic Deployment
----------------

Assume that hosts A and B can exchange unicast datagram (UDPv4)
traffic, but multicast between them is blocked by firewalls or
routers.

On each host, any number of processes that use a multicast group can exchange
datagrams with each other.  This process (the repeater) will bridge those
datagrams to the other host, and send that other host's datagrams to the local
mulitcast group.

On host A: ``repeater.js --group 239.255.0.1:7400 --uport 9999 --send B``
On host B: ``repeater.js --group 239.255.0.1:7400 --uport 9999 --send A``

A single source datagram flows through the system as follows:
1. Some process on host A sends to the group address 239.255.0.1:7400
2. The repeater on host A receives the datagram and sends the same payload
   to the repeater on host B using the unicast port 9999
3. The repeater on host B receives this new datagram on port 9999 and sends a
   datagram with the same payload to the group address 239.255.0.1:7400
4. One or more processes on host B receive this datagram.  Except for the source
   address, they are unaware that the repeater was used to send this data.

Extended Deployment
-------------------

The example can be extended to more than two hosts by repeating the ``--send``
argument to provide addresses for all of the other nodes.

Using this same example, what we've been calling host A could be one host
within a subnet.  In this scenario, hosts A1 and A2 can exchange multicast
traffic, as can B1 and B2.  However, B1 can't multicast to A1, etc.  Using the
same deployment as above, the repeater on A1 can also serve A2 and the repeater
on B1 can serve B2.  Each multicast domain needs one repeater.

Integration with Google Cloud Platform (GCP)
--------------------------------------------

Instead of (or in addition to) using ``--send`` to manually specify
destination addresses, one can use ``--gcp zone:group`` which will
cause the repeater to use GCP's API to find the IP addresses of all
other instances in the group.  The instances need to have read
permissions for the Compute Engine API.

Integration with Microsoft Azure
--------------------------------

Instead of (or in addition to) using ``--send`` to manually specify
destination addresses, one can use ``--azure resource_group`` which
will cause the repeater to use Azure's API to find the IP addresses of
all other instances in the resource group.  The VM running the
repeater needs to have read access to the network interfaces in the
resource group.

Integration with Amazon Web Services (AWS)
------------------------------------------

Instead of (or in addition to) using ``--send`` to manually specify
destination addresses, one can use ``--aws tag`` which will cause the
repeater to use AWS's API to find the IP addresses of all other
instances with the given tag.  The VM running the repeater needs to
have permission to `DescribeInstances`.
