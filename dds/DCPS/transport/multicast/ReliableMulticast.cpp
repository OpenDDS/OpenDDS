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
  // messages for a specific passive peer. We will always select
  // passive peer based on the remote peer assigned when the
  // DataLink was created:
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
  // a warning and return. In the future, it may be worthwhile to
  // allow a DataLink to initiate the removal of an association
  // via the TransportSendListener interface.
  ACE_ERROR((LM_WARNING,
             ACE_TEXT("(%P|%t) WARNING: ")
             ACE_TEXT("SynWatchdog::on_timeout: ")
             ACE_TEXT("timed out handshaking with remote peer: 0x%x!\n"),
             this->link_->remote_peer()));
}

ReliableMulticast::ReliableMulticast(MulticastTransport* transport,
                                     ACE_INT32 local_peer,
                                     ACE_INT32 remote_peer)
  : MulticastDataLink(transport, local_peer, remote_peer),
    syn_watchdog_(this),
    acked_(false)
{
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

  // MULTICAST_SYN control messages are always positively
  // acknowledged by a matching remote peer. In the future
  // it may be worthwhile to predicate this on association
  // status to prevent potential denial of service attacks.
  send_synack(remote_peer);
}

void
ReliableMulticast::send_syn(ACE_INT32 remote_peer)
{
  size_t len = sizeof (this->local_peer_) +
               sizeof (remote_peer);

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
  serializer << remote_peer;

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

  // The handshake is now complete; we have verified that the
  // passive peer is indeed accepting (and responding) to samples
  // reliably. Adjust the acked flag and force the TransportImpl
  // to re-evaluate any pending associations it has queued:
  this->acked_ = true;
  this->transport_->check_fully_association();
}

void
ReliableMulticast::send_synack(ACE_INT32 remote_peer)
{
  size_t len = sizeof (this->local_peer_) +
               sizeof (remote_peer);

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

bool
ReliableMulticast::join_i(const ACE_INET_Addr& /*group_address*/, bool active)
{
  if (!active) return true; // passive peers are done

  // Active peers must initiate a handshake to verify the DataLink
  // is indeed reliable. We do this by scheduling a watchdog timer
  // to broadcast MULTICAST_SYN control messages to passive peers
  // at fixed intervals. This process must be executed using the
  // transport reactor thread to prevent blocking.
  ACE_Reactor* reactor = get_reactor();
  if (reactor == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ReliableMulticast::join_i: ")
                      ACE_TEXT("NULL reactor reference!\n")),
                     false);
  }

  if (!this->syn_watchdog_.schedule(reactor)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ReliableMulticast::join_i: ")
                      ACE_TEXT("failed to schedule watchdog timer!\n")),
                     false);
  }

  return true;
}

void
ReliableMulticast::leave_i()
{
  this->syn_watchdog_.cancel();
}

} // namespace DCPS
} // namespace OpenDDS
