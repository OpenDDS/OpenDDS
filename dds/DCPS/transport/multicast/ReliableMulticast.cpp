/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReliableMulticast.h"
#include "MulticastTransport.h"

#include "dds/DCPS/Serializer.h"

namespace OpenDDS {
namespace DCPS {

NakWatchdog::NakWatchdog(ReliableMulticast* link)
  : DataLinkWatchdog<ReliableMulticast>(link)
{
}

ACE_Time_Value
NakWatchdog::next_interval()
{
  // TODO In the future, it may be worthwhile to introduce
  // a random backoff and NAK suppression to prevent NAK
  // implosions in large multicast groups.
  MulticastConfiguration* config = this->link_->config();
  return config->nak_interval_;
}

void
NakWatchdog::on_interval(const void* /*arg*/)
{
  // Initiate resend by broadcasting MULTICAST_NAK control
  // messages to remote peers from which we are missing data:
  this->link_->send_naks();
}


SynWatchdog::SynWatchdog(ReliableMulticast* link)
  : DataLinkWatchdog<ReliableMulticast>(link)
{
}

ACE_Time_Value
SynWatchdog::next_interval()
{
  // TODO In the future, it may be worthwhile to introduce
  // an exponential backoff to prevent SYN flooding in large
  // multicast groups.
  MulticastConfiguration* config = this->link_->config();
  return config->syn_interval_;
}

void
SynWatchdog::on_interval(const void* /*arg*/)
{
  // Initiate handshake by broadcasting MULTICAST_SYN control
  // messages for a specific remote peer (HINT: this is the
  // same remote peer assigned when the DataLink was created).
  this->link_->send_syn();
}

ACE_Time_Value
SynWatchdog::next_timeout()
{
  MulticastConfiguration* config = this->link_->config();
  return config->syn_timeout_;
}

void
SynWatchdog::on_timeout(const void* /*arg*/)
{
  // There is no recourse if a link is unable to handshake; log
  // an error and return.
  //
  // TODO In the future, it may be worthwhile to allow a DataLink
  // to initiate the removal of an association via the
  // TransportSendListener interface.
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) ERROR: ")
             ACE_TEXT("SynWatchdog::on_timeout: ")
             ACE_TEXT("timed out handshaking with remote peer: 0x%x!\n"),
             this->link_->remote_peer()));
}


ReliableMulticast::ReliableMulticast(MulticastTransport* transport,
                                     MulticastPeer local_peer,
                                     MulticastPeer remote_peer)
  : MulticastDataLink(transport,
                      local_peer,
                      remote_peer),
    acked_(false),
    nak_watchdog_(this),
    syn_watchdog_(this)
{
}

bool
ReliableMulticast::header_received(const TransportHeader& header)
{
  this->recvd_header_ = header;

  SequenceMap::iterator it(this->sequences_.find(header.source_));
  if (it != this->sequences_.end()) {
    // Update last seen sequence for remote peer; return false if we
    // have already seen this packet (prevents duplicate delivery):
    return it->second.update(header.sequence_);
  }

  return true;
}

void
ReliableMulticast::sample_received(ReceivedDataSample& sample)
{
  switch(sample.header_.message_id_) {
  case TRANSPORT_CONTROL: {
    ACE_Message_Block* message = sample.sample_;

    switch (sample.header_.submessage_id_) {
    case MULTICAST_SYN:
      syn_received(message);
      break;

    case MULTICAST_SYNACK:
      synack_received(message);
      break;

    case MULTICAST_NAK:
      nak_received(message);
      break;

    case MULTICAST_NAKACK:
      nakack_received(message);
      break;

    default:
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ")
                 ACE_TEXT("ReliableMulticast::sample_received: ")
                 ACE_TEXT("unknown TRANSPORT_CONTROL submessage: 0x%x!\n"),
                 sample.header_.submessage_id_));
    }
  } break;

  case SAMPLE_ACK:
    ack_received(sample);
    break;

  default:
    data_received(sample);
  }
}

bool
ReliableMulticast::acked()
{
  return this->acked_;
}

void
ReliableMulticast::syn_received(ACE_Message_Block* message)
{
  TAO::DCPS::Serializer serializer(
    message, this->transport_->swap_bytes());

  MulticastPeer remote_peer;  // sent as local_peer
  MulticastPeer local_peer;   // sent as remote_peer

  serializer >> remote_peer;
  serializer >> local_peer;

  // Ignore message if not destined for us:
  if (local_peer != this->local_peer_) return;

  // Insert remote peer into sequence map. This establishes a baseline
  // to check for reception gaps during delivery of sample data:
  SequenceNumber value(this->recvd_header_.sequence_);
  
  std::pair<SequenceMap::iterator, bool> pair = this->sequences_.insert(
    SequenceMap::value_type(remote_peer, DisjointSequence(value)));
  if (pair.first == this->sequences_.end()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("ReliableMulticast::syn_received: ")
               ACE_TEXT("failed to insert sequence for remote peer: 0x%x!\n"),
               remote_peer));
    return;
  }

  // MULTICAST_SYN control messages are always positively
  // acknowledged by a matching remote peer.
  //
  // TODO In the future it may be worthwhile to predicate
  // this behavior on association status to prevent potential
  // denial of service attacks.
  send_synack(remote_peer);
}

void
ReliableMulticast::send_syn()
{
  size_t len = sizeof (this->local_peer_)
             + sizeof (this->remote_peer_);

  ACE_Message_Block* data;
  ACE_NEW_NORETURN(data, ACE_Message_Block(len));
  if (data == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("ReliableMulticast::send_syn: ")
               ACE_TEXT("failed to allocate message!\n")));
    return;
  }

  TAO::DCPS::Serializer serializer(
    data, this->transport_->swap_bytes());

  serializer << this->local_peer_;
  serializer << this->remote_peer_;

  ACE_Message_Block* message =
    create_control(MULTICAST_SYN, data);

  int error;
  if ((error = send_control(message)) != SEND_CONTROL_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("ReliableMulticast::send_syn: ")
               ACE_TEXT("send_control failed: %d!\n"),
               error));
    return;
  }
}

void
ReliableMulticast::synack_received(ACE_Message_Block* message)
{
  TAO::DCPS::Serializer serializer(
    message, this->transport_->swap_bytes());

  MulticastPeer remote_peer;  // sent as local_peer
  MulticastPeer local_peer;   // sent as remote peer

  serializer >> remote_peer;
  serializer >> local_peer;

  // Ignore message if not destined for us:
  if (local_peer != this->local_peer_) return;

  // 2-way handshake is complete; we have verified that the remote
  // peer is indeed sending/receiving data reliably. Adjust the
  // acked flag and force the TransportImpl to re-evaluate any
  // pending associations it has queued:
  this->syn_watchdog_.cancel();
  this->acked_ = true;

  this->transport_->check_fully_association();
}

void
ReliableMulticast::send_synack(MulticastPeer remote_peer)
{
  size_t len = sizeof (this->local_peer_)
             + sizeof (remote_peer);

  ACE_Message_Block* data;
  ACE_NEW_NORETURN(data, ACE_Message_Block(len));
  if (data == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("ReliableMulticast::send_synack: ")
               ACE_TEXT("failed to allocate message!\n")));
    return;
  }

  TAO::DCPS::Serializer serializer(
    data, this->transport_->swap_bytes());

  serializer << this->local_peer_;
  serializer << remote_peer;

  ACE_Message_Block* message =
    create_control(MULTICAST_SYNACK, data);

  int error;
  if ((error = send_control(message)) != SEND_CONTROL_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("ReliableMulticast::send_synack: ")
               ACE_TEXT("send_control failed: %d!\n"),
               error));
    return;
  }
}

void
ReliableMulticast::nak_received(ACE_Message_Block* message)
{
  // TODO implement
}

void
ReliableMulticast::send_nak(MulticastPeer remote_peer,
                            MulticastSequence low,
                            MulticastSequence high)
{
  size_t len = sizeof (this->local_peer_)
             + sizeof (remote_peer)
             + sizeof (low)
             + sizeof (high);

  ACE_Message_Block* data;
  ACE_NEW_NORETURN(data, ACE_Message_Block(len));
  if (data == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("ReliableMulticast::send_nak: ")
               ACE_TEXT("failed to allocate message!\n")));
    return;
  }

  TAO::DCPS::Serializer serializer(
    data, this->transport_->swap_bytes());

  serializer << this->local_peer_;
  serializer << remote_peer;
  serializer << low;
  serializer << high;

  ACE_Message_Block* message =
    create_control(MULTICAST_NAK, data);

  int error;
  if ((error = send_control(message)) != SEND_CONTROL_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("ReliableMulticast::send_nak: ")
               ACE_TEXT("send_control failed: %d!\n"),
               error));
    return;
  }
}

void
ReliableMulticast::send_naks()
{
  for (SequenceMap::iterator it(this->sequences_.begin());
       it != this->sequences_.end(); ++it) {
    
    if (!it->second.disjoint()) continue; // nothing to NAK

    for (DisjointSequence::range_iterator range(it->second.range_begin());
         range != it->second.range_end(); ++range) {
      // Send MULTICAST_NAK request to remote peer. The peer should
      // respond with either a resend of the missing data or a
      // MULTICAST_NAKACK indicating the data is no longer available.
      send_nak(it->first, range->first, range->second);
    }
  }
}

void
ReliableMulticast::nakack_received(ACE_Message_Block* message)
{
  // TODO implement
}

void
ReliableMulticast::send_nakack(MulticastPeer remote_peer,
                               MulticastSequence low,
                               MulticastSequence high)
{
  // TODO implement
}

bool
ReliableMulticast::join_i(const ACE_INET_Addr& /*group_address*/, bool active)
{
  ACE_Reactor* reactor = get_reactor();
  if (reactor == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ReliableMulticast::join_i: ")
                      ACE_TEXT("NULL reactor reference!\n")),
                     false);
  }

  // A watchdog timer is scheduled to periodically check for gaps in
  // received data. If a gap is discovered, MULTICAST_NAK control
  // messages will be broadcasted to initiate resends from
  // responsible remote peers.
  //
  // Currently, this transport relies on LIFESPAN to periodically
  // send heartbeat messages to aid in detecting reception gaps.
  //
  // TODO In the future it may be worthwhile to implement an FEC
  // mechanism to reduce repair requests and provide an additional
  // source from which to detect gaps.
  if (!this->nak_watchdog_.schedule(reactor)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ReliableMulticast::join_i: ")
                      ACE_TEXT("failed to schedule NAK watchdog timer!\n")),
                     false);
  }

  if (active) {
    // Active peers schedule a watchdog timer to initiate a 2-way
    // handshake to verify that passive endpoints can send/receive
    // data reliably. This process must be executed using the
    // transport reactor thread to prevent blocking.
    if (!this->syn_watchdog_.schedule(reactor)) {
      this->nak_watchdog_.cancel();
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("ReliableMulticast::join_i: ")
                        ACE_TEXT("failed to schedule SYN watchdog timer!\n")),
                       false);
    }
  }

  return true;
}

void
ReliableMulticast::leave_i()
{
  this->syn_watchdog_.cancel();
  this->nak_watchdog_.cancel();
}

} // namespace DCPS
} // namespace OpenDDS
