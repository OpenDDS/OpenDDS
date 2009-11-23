/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReliableMulticast.h"

#include "dds/DCPS/Serializer.h"

namespace OpenDDS {
namespace DCPS {

SynWatchdog::SynWatchdog(ReliableMulticast* link)
  : DataLinkWatchdog<ReliableMulticast>(link)
{
}

ACE_Time_Value
SynWatchdog::next_interval()
{
  MulticastConfiguration* config = this->link_->config();

  // In the future, it may be worthwhile to introduce an
  // exponential backoff to prevent potential SYN flooding in
  // large multicast groups.
  ACE_Time_Value interval;
  interval.msec(config->syn_interval_);

  return interval;
}

bool
SynWatchdog::on_interval(const void* /*arg*/)
{
  // Initiate handshake by broadcasting MULTICAST_SYN control
  // messages for a specific remote peer (this is the same
  // remote peer assigned when the DataLink was created).
  this->link_->send_syn(this->link_->remote_peer());
  return true;  // reschedule
}

ACE_Time_Value
SynWatchdog::next_timeout()
{
  MulticastConfiguration* config = this->link_->config();
  
  ACE_Time_Value timeout;
  timeout.msec(config->syn_timeout_);

  return timeout;
}

void
SynWatchdog::on_timeout(const void* /*arg*/)
{
  // There is no recourse if a link is unable to handshake; log
  // an error and return. In the future, it may be worthwhile to
  // allow a DataLink to initiate the removal of an association
  // via the TransportSendListener interface.
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) ERROR: ")
             ACE_TEXT("SynWatchdog::on_timeout: ")
             ACE_TEXT("timed out handshaking with remote peer: 0x%x!\n"),
             this->link_->remote_peer()));
}

//

NakWatchdog::NakWatchdog(ReliableMulticast* link)
  : DataLinkWatchdog<ReliableMulticast>(link)
{
}

ACE_Time_Value
NakWatchdog::next_interval()
{
  MulticastConfiguration* config = this->link_->config();

  // In the future, it may be worthwhile to introduce an
  // exponential backoff to prevent NAK implosions in large
  // multicast groups.
  ACE_Time_Value interval;
  interval.msec(config->nak_interval_);

  return interval;
}

bool
NakWatchdog::on_interval(const void* /*arg*/)
{
  // Initiate resend by broadcasting MULTICAST_NAK control
  // messages to any peers from which we have missing data.
  this->link_->send_naks();
  return true;  // reschedule
}

//

ReliableMulticast::ReliableMulticast(MulticastTransport* transport,
                                     ACE_INT32 local_peer,
                                     ACE_INT32 remote_peer)
  : MulticastDataLink(transport,
                      local_peer,
                      remote_peer),
    acked_(false),
    syn_watchdog_(this),
    nak_watchdog_(this)
{
}

bool
ReliableMulticast::header_received(const TransportHeader& header)
{
  ACE_WRITE_GUARD_RETURN(ACE_RW_Thread_Mutex,
                         guard,
                         this->passive_lock_,
                         true);
  
  this->last_sequence_ = header.sequence_;

  SequenceMap::iterator it = this->sequences_.find(header.source_);
  if (it != this->sequences_.end()) {
    // Update last known sequence for active peer; return false
    // if we have already seen this packet:
    return it->second.update(this->last_sequence_);
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
    }
  } break;

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

  ACE_INT32 remote_peer;  // sent as local_peer
  ACE_INT32 local_peer;   // sent as remote_peer

  serializer >> remote_peer;
  serializer >> local_peer;

  // Ignore message if not destined for us:
  if (local_peer != this->local_peer_) return;

  // Insert active peer into sequence map; this establishes a
  // baseline from which to begin verifying sequence numbers for
  // MULTICAST_NAK requests.
  {
    ACE_WRITE_GUARD(ACE_RW_Thread_Mutex,
                    guard,
                    this->passive_lock_);

    std::pair<SequenceMap::iterator, bool> pair = this->sequences_.insert(
      SequenceMap::value_type(remote_peer, DisjointSequence(this->last_sequence_)));
    if (pair.first == this->sequences_.end()) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("ReliableMulticast::syn_received: ")
                 ACE_TEXT("failed to insert sequence for remote peer: 0x%x!\n"),
                 remote_peer));
      return;
    }
  }

  // MULTICAST_SYN control messages are always positively
  // acknowledged by a matching remote peer. In the future
  // it may be worthwhile to predicate this on association
  // status to prevent potential denial of service attacks.
  send_synack(remote_peer);
}

void
ReliableMulticast::send_syn(ACE_INT32 remote_peer)
{
  size_t len = sizeof (this->local_peer_)
             + sizeof (remote_peer_);

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
  serializer << remote_peer_;

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

  ACE_INT32 remote_peer;  // sent as local_peer
  ACE_INT32 local_peer;   // sent as remote peer

  serializer >> remote_peer;
  serializer >> local_peer;

  // Ignore message if not destined for us:
  if (local_peer != this->local_peer_) return;

  this->syn_watchdog_.cancel();

  // 2-way handshake is complete; we have verified that the passive
  // peer is indeed sending/receiving data reliably. Adjust the
  // acked flag and force the TransportImpl to re-evaluate any
  // pending associations it has queued:
  this->acked_ = true;
  this->transport_->check_fully_association();
}

void
ReliableMulticast::send_synack(ACE_INT32 remote_peer)
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
ReliableMulticast::send_nak(ACE_INT32 remote_peer,
                            ACE_INT16 low,
                            ACE_INT16 high)
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
  ACE_READ_GUARD(ACE_RW_Thread_Mutex,
                 guard,
                 this->passive_lock_);

  for (SequenceMap::iterator it = this->sequences_.begin();
       it != this->sequences_.end(); ++it) {
    if (!it->second.disjoint()) continue; // nothing to NAK

    DisjointSequence::RangeSet ranges;
    if (!it->second.range(ranges, this->config_->nak_repair_size_)) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("ReliableMulticast::send_nak: ")
                 ACE_TEXT("failed to collect NAK ranges for remote peer: 0x%x!\n"),
                 it->first));
      return;
    }

    for (DisjointSequence::RangeSet::iterator range = ranges.begin();
         range != ranges.end(); ++range) {
      // Send MULTICAST_NAK request to active peer. The peer should
      // respond with either a re-send of the missing data or a
      // MULTICAST_NAKACK indicating the data is no longer available.
      send_nak(it->first, range->first.value_, range->second.value_);
    }
  }
}

void
ReliableMulticast::nakack_received(ACE_Message_Block* message)
{
  // TODO implement
}

void
ReliableMulticast::send_nakack(ACE_INT32 remote_peer,
                               ACE_INT16 low,
                               ACE_INT16 high)
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

  if (active) {
    // Active peers initiate a 2-way handshake to verify that passive
    // endpoints can send/receive data reliably. A watchdog timer is
    // scheduled to broadcast MULTICAST_SYN control messages at fixed
    // intervals. This process must be executed using the transport
    // reactor thread to prevent blocking.
    if (!this->syn_watchdog_.schedule(reactor)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("ReliableMulticast::join_i: ")
                        ACE_TEXT("failed to schedule watchdog timer!\n")),
                       false);
    }
  
  } else {
    // Passive peers simply schedule a watchdog timer to handle sending
    // MULTICAST_NAK requests for missing data.
    if (!this->nak_watchdog_.schedule(reactor)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("ReliableMulticast::join_i: ")
                        ACE_TEXT("failed to schedule watchdog timer!\n")),
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
