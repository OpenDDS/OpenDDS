==========
RTPS Relay
==========

Motivation
==========

The designers of DDS and related technologies like RTPS made
assumptions about networks that are not generally valid.  The two
assumptions motivating this work are multicast and that participants
are not subject to network address translation.  Most cloud providers
do not support multicast.  Consequently, deployments of OpenDDS in the
cloud cannot use RTPS discovery and instead must either use the InfoRepo,
which has its own issues, or solve discovery in some other way.
Network Address Translation (NAT) is a common technique for
establishing private networks that have Internet connectivity.  For
example, many homes and small business have networks deployed behind a
firewall that performs NAT.  An OpenDDS deployment that spans NATs
will face communication problems since the network addresses know by
the participants are only valid behind the NAT and not suitable for
communication across the NAT.

Scope
=====

There are three types of communication paths between DDS Participants.
Participants on the same network (LAN) can communicate directly.  In
situations were one participant is behind a NAT firewall, there exists
the possibility that the participants can communicate by using an
address on the public side of the NAT.  A protocol like ICE may be
used to determine these addresses and establish connectivity.
Finally, participants with access to a common peer can use the peer to
relay messages.  The scope of this work is to develop such a relay for
RTPS messages.

Objectives
==========

In addition to the basic functionality of relaying RTPS messages, we
have the following objectives:

* Scalability - the RTPS relay should be able to scale horizontally.
* Efficiency - The relay should not forward messages to participants
  that are not interested in those messages.

Architecture
============

The following diagram shows the major components of an RtpsRelay::

    +-----------------------------------------------------+   +-----------------------------------------------------+
    | RtpsRelay                                           |   | RtpsRelay                                           |
    |                                                     |   |                                                     |
    |                              +-------------------+  |   |                              +-------------------+  |
    |                              | Relay Participant |<-+---+----------------------------->| Relay Participant |  |
    |                              +-------------------+  |   |                              +-------------------+  |
    |                                                     |   |                                                     |
    |                               +-----------------+   |   |                               +-----------------+   |
    |                               | Horizontal Port |<--+---+------------------------------>| Horizontal Port |   |
    |                               +-----------------+   |   |                               +-----------------+   |
    |                                                     |   |                                                     |
    | +-------------------------+    +---------------+    |   | +-------------------------+    +---------------+    |
    | | Application Participant |<-->| Vertical Port |    |   | | Application Participant |<-->| Vertical Port |    |
    | +-------------------------+    +---------------+    |   | +-------------------------+    +---------------+    |
    |                                        ^            |   |                                        ^            |
    |                                        |            |   |                                        |            |
    +----------------------------------------+------------+   +----------------------------------------+------------+
                                             |                                                         |
                                      +------+------+                                           +------+------+
                                      | Participant |                                           | Participant |
                                      +-------------+                                           +-------------+

* Relay Participant - A DDS Participant that the relays use to
  exchange information about readers and writers.
* Horizontal Port - A UDP port that the relays use to send and receive
  RTPS messages with other relays.
* Vertical Port - A UDP port that the relays use to send and receive
  RTPS messages with participants.
* Application Participant - A DDS Participant in the application's
  domain.  This allows the relay to participate in (secure) discovery.
  An Application Participant only communicates with the participants that
  use the corresponding relay.

A participant should use a single relay.  When a participant sends a
message to a relay, a couple of things happen.  First, if the
participant is behind a NAT, then the NAT bindings are updated to
allow traffic from the relay back to the participant.  Second, the
relay records the public address of the participant.  Third, it
forwards messages to the Application Participant so that that relay
can discover the readers and writers in the participant.  Once a relay
discovers this information, it shares it with the other relays via DDS
topics in the Relay Participant.  Fourth, the relay makes a forwarding
decision that might involve sending the message to other participants
that are using the relay in question or forwarding the message to
other relays because they have participants that should receive the
message.

If a relay receives a message from another relay, it forwards the
message to any participants that it serves that should be recipients
of the message.

The horizontal port and vertical ports are replicated three times for
SPDP, SEDP, and the data transports.

Every transport (in OpenDDS terminology) must send at least one
message to its associated relay so the relay can learn its public
address.  Realistically, each transport should send messages
periodically to refresh the NAT bindings.  For SPDP, this is generally
not a problem since SPDP broadcasts are periodic.  This is a potential
problem for SEDP even though it has not be observed in practice
because SEDP traffic is linked to the periodic SPDP traffic.  This is
a confirmed problem for data readers since the RTPS protocol does not
require a reader to send any messages.  To address this problem,
OpenDDS periodically sends empty RTPS messages to the relay for
readers.

Arguments
=========

* :code:`-HorizontalAddress` - An IP:port pair indicating the address
  to use for the horizontal SPDP port.  The SEDP port will listen at
  port + 1 and the data port will listen at port + 2.  The default
  port is 11444.
* :code:`-VerticalAddress` - An IP:port pair indicating the address to
  use for the vertical SPDP port.  The SEDP port will listen at port +
  1 and the data port will listen at port + 2.  The default port
  is 444.
* :code:`-RelayDomain` - The DDS domain to use for the Relay Participant.
  The default is 0.
* :code:`-ApplicationDomain` - The DDS domain to use for Application Participant.
  The default is 1.
* :code:`-Lifespan` - Time in seconds after which the relay will purge
  IP:port information for an inactive participant.  The default is 60
  seconds.

Participant Configuration
=========================

To use a relay, a participant must use the following configuration options:

* :code:`SpdpRtpsRelayAddress` - Vertical SPDP IP:port of an RtpsRelay.
* :code:`SedpRtpsRelayAddress` - Vertical SEDP IP:port of an RtpsRelay.
* :code:`DataRtpsRelayAddress` - Vertical data IP:port of an RtpsRelay.

A participant may change the period for sending empty RTPS messages
for readers by adjusting the :code:`heartbeat_period` configuration
option.

Deployment Notes
================

Many UDP load balancers won't work with the RtpsRelay in scenarios
where the participants are subject to network address translation.
Conceptually, there is no problem creating a UDP load balancer that is
serviced by a pool of RtpsRelays.  The load balancers in question
would forward the datagram from a participant to a relay without
difficulty.  However, the relays could not send datagrams back to the
participants because the NAT bindings set up by the outgoing messages
are expecting return traffic from the UDP load balancer IP.  For
return traffic to flow, the relays would need to spoof the load
balancer's IP as the source address.  This would require hacking on a
variety of levels to the point that it is not a feasible option.

From the previous result, we conclude that a pool of relays must all
have a public IP address so they can exchange messages with
participants.  Load balancing can be accomplished by having the
participants choose a relay according to some load balancing
algorithm.  To this end, one can run a simple web server on each relay
machine that serves the vertical addresses and ports for the relay on
the same machine.  These webservers can be placed behind a load
balancer.  A participant, then, contacts the load balancer for the
webservers to find the relay to use.

Limitations and Future Work
===========================

* Secure SPDP messages.  The routing decisions made by the relays are
  driven by the groups declared in SPDP messages.  These messages are
  unencrypted and unauthenticated.
