Interactive Connectivity Establishment (ICE) for RTPS
=====================================================

ICE is a protocol that allows two peers to exchange datagrams when one
or both are behind a firewall that performs network address
translation.  ICE is documented in IETF RFC 8445
https://www.rfc-editor.org/info/rfc8445.  ICE relies on the Session
Traversal Utilities for NAT (STUN) protocol which is documented in
IETF RFC 5389 https://www.rfc-editor.org/info/rfc5389.

ICE can be used for UDP and TCP connections.  The implementation in
OpenDDS is only concerned with UDP as it is used for the UDP variant
of RTPS.  Currently, OpenDDS uses three sockets for RTPS: one for
SPDP, one for SEDP, and another for the transport.  ICE could be used
for all three but is currently only implemented for SEDP and the
transport since they share the transport code.  Furthermore, there is
not a clear need to run ICE for SPDP.  ICE can also be used with a
TURN server as a generic approach to providing point-to-point
connectivity.  Instead of using a TURN server, developers are advised
to the RtpsRelay to provide similar functionality.

Overview of Network Address Translation (NAT)
=============================================

When a firewall that performs network address translation receives an
outbound packet from a host, it substitutes the source IP and port
with one of its external IPs and a port that it assigns.  The
relationship between the internal host IP and port and the external IP
and port is called a NAT binding.  When the firewall receives an
inbound packet, it performs the reverse operation substituting the
host's IP address and port for the destination address assuming that a
NAT binding exists.

Some approaches to NAT are compatible with ICE and some are not.  The
most permissive NATs forward any packet sent to the public side of the
binding to the host.  A more restrictive NAT records the destination IP
that caused the NAT binding to be generated and only accepts packets
from the same IP.  An even more restrictive NAT may record and enforce
the port as well.  Due to the way NAT is implemented, ICE may not work
in every scenario.

NAT bindings are limited in time.  First, a firewall may rate limit
the creation of NAT bindings.  The ICE documentation recommends at
least 50ms between transactions that may create NAT bindings.  Second,
a firewall will eventually purge the NAT binding after a timeout.
Most sources report that most firewalls use a timeout of 15 to 30
seconds.

Overview of STUN
================

STUN is a simple protocol consisting of a single message class
(binding) and four message types.  A *binding request* illicits either
a *binding response* or an *error*.  A binding response contains the
source IP address and port as perceived by the STUN server when it
received the binding request.  If there are no NATting firewalls on
the route the STUN server, then the address returned by the STUN
server corresponds to the address of the host.  If there is a NATting
firewall on the route, then the IP address that is returned belongs to
the outer-most NATting firewall that separates the client from the
server.  A *binding indication* is used to refresh a NAT binding and
is dropped by the server.

Overview of ICE
===============

The ICE protocol begins with the assumption that the peers know they
should communicate.  The portion of the peer that is responsible for
executing the ICE protocol is called the *ICE agent*.  A agent is
either the controlling agent or the controlled agent with the
difference being that the controlling agent can nominate.  First, the
peers generate *candidates* which, among other things, contain a type,
a priority, and an IP address and port.  There are four candidate
types, three of which are collected in this phase.

* A *host candidate* is based on the IP addresses of the network interfaces of the host
* A *server reflexive candidate* represent the public address of the host (described below)
* A *relayed candidate* is acquired from a TURN server and not used in OpenDDS

A server reflexive candidate is determined by sending a STUN binding
request to a public STUN server.  When the STUN server receives the
request, it copies the source address of the request into the binding
response.  This allows the host to determine its public IP address,
i.e., the public IP address of the outer-most NATting firewall.

The peers exchange candidates using a side channel.  Once a peer
receives the candidates, it pairs the candidates with its own to
create prioritized candidate pairs and starts performing connectivity
checks on the candidate pairs.  The connectivity checks may generate a
fourth type of candidate called a *peer reflexive address*.  The
follow diagram illustrates how this happens.

::

    +--------+     +------------+     +------------+     +--------+
    | Peer A |<--->| Firewall A |<--->| Firewall B |<--->| Peer B |
    +--------+     +------------+     +------------+     +--------+

  1 --------------------------------->

  2 <--------------------------------------------------------------
    -------------------------------------------------------------->

  3 -------------------------------------------------------------->
    <--------------------------------------------------------------

Peer A sends a binding request to the server reflexive address of Peer B (1).
This is most likely dropped by Firewall B since no NAT binding exists.
However, it creates a NAT binding in Firewall A.
Peer B sends a binding request to the server reflexive address of Peer A (2).
Assuming a compatible firewall, Firewall A forwards the packet to Peer A based on the NAT binding created in (1) and Peer A sends a binding response.
The binding response contains the public IP and port for Peer B.
Finally, Peer A sends a binding request back to Peer B and receives a binding response.

Once the connectivity checks complete, the controlling agent nominates
a candidate pair and indicates this to the contolled agent.  Finally,
the peers use their respective nominated candidate to exchange data.
Also, the peers may send binding indications and/or requests to keep
the NAT bindings in place.

Implementation
==============

To apply ICE to OpenDDS, we determined that SPDP would be the
side-channel for SEDP and that SEDP would be the side-channel for the
normal RTPS port.  To do this, we added two parameters to the RTPS
protocol for exchanging general ICE information (``IceGeneral_t``) and
candidates (``IceCandidate_t``).

ICE is compiled into the RTPS library when security is enabled due a
dependency on random number generation and HMAC.  There is a singleton
ICE agent that works with endpoints.  Each endpoint corresponds to a
RTPS UDP transport instance which services both SEDP and normal RTPS.
The agent is a singleton to rate limit outgoing STUN messages
according to the recommendation in ICE.

Security
========

ICE mandates the use of short-term credentials.  As part of the
candidate exchange, each agent includes a username and password.  A
STUN message sent to an agent must be signed (see the Message
Integrity attribute in STUN) with the password of the agent.  This
creates a dependency on OpenSSL.  The password should be changed
periodically, e.g., 5 minutes, to prevent replay attacks.  Since the
password is sensitive, it should not be sent in the clear.  This is
not a problem for SEDP with security but does present a problem for
SPDP.  We can consider the two scenarios of using the RtpsRelay and
ordinary local multicast.  For the RtpsRelay, DTLS should be used to
secure the SPDP messages.  This has not been implemented yet.  The
local multicast scenario is a bit more interesting since we can
consider both secure and unsecure deployments.  However, if the agent
is received via local multicast, then ICE is probably not necessary.
To summarize, the agent info is only necessary when sending to the
RtpsRelay and it should be secured by DTLS.

Limitations
===========

* A writer or reader that is using ICE can only be associated with one
  (RTPS UDP) transport.  The RTPD UDP data link code is at the crux of
  the problem.
* There is no support for IPV6.
