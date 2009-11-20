/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReliableMulticast.h"

#include "ace/OS_NS_time.h"

#include "dds/DCPS/Serializer.h"

namespace OpenDDS {
namespace DCPS {

SynHandler::SynHandler(const ACE_Time_Value& deadline)
  : deadline_(deadline)
{
}

int
SynHandler::handle_timeout(const ACE_Time_Value& now,
                           const void* arg)
{
  if (now > this->deadline_) {
    reactor()->cancel_timer(this);
    delete this;

    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("SynHandler::handle_timeout: ")
                      ACE_TEXT("deadline exceeded; giving up!\n")),
                     0);
  }

  ReliableMulticast* link =
    static_cast<ReliableMulticast*>(const_cast<void*>(arg));  // safe

  // Initiate a handshake by sending a MULTICAST_SYN control
  // message to a specific remote peer. In this case, we will
  // always select the remote (passive) peer assigned when the
  // DataLink was created:
  link->send_syn(link->remote_peer());

  return 0;
}

ReliableMulticast::ReliableMulticast(MulticastTransport* transport,
                                     ACE_INT32 local_peer,
                                     ACE_INT32 remote_peer)
  : MulticastDataLink(transport, local_peer, remote_peer),
    acked_(false),
    syn_timer_id_(-1)
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
  // acknowledged by a matching remote peer, regardless of
  // association status. This prevents a number of known race
  // conditions during association which could prevent a
  // successful handshake.
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
               ACE_TEXT("unable to create message!\n")));
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

  cancel_syn_timer();
 
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
               ACE_TEXT("unable to create message!\n")));
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
  if (!active) return true; // passive peers are finished

  // Active peers must initiate a handshake to verify that the
  // reservation is reliable. We do this by setting a bounded
  // timer to broadcast MULTICAST_SYN control messages to the
  // passive peer. This process must be executed using the
  // transport reactor thread to prevent uneccessary blocking in
  // the TransportImpl::find_or_create_datalink call.

  ACE_Reactor* reactor = get_reactor();
  if (reactor == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ReliableMulticast::join_i: ")
                      ACE_TEXT("NULL reactor reference!\n")),
                     false);
  }

  ACE_Time_Value deadline(ACE_OS::gettimeofday());
  deadline += this->config_->syn_timeout();
  
  ACE_Time_Value interval = this->config_->syn_interval();
  
  this->syn_timer_id_ =
    reactor->schedule_timer(new SynHandler(deadline),
                            this,
                            ACE_Time_Value::zero,
                            interval);
  if (this->syn_timer_id_ == -1) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ReliableMulticast::join_i: ")
                      ACE_TEXT("failed to schedule timer!\n")),
                     false);
  }

  return true;
}

void
ReliableMulticast::cancel_syn_timer()
{
  if (this->syn_timer_id_ == -1) return;

  ACE_Reactor* reactor = get_reactor();
  if (reactor == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("ReliableMulticast::leave_i: ")
               ACE_TEXT("NULL reactor reference!\n")));
    return;
  }

  reactor->cancel_timer(this->syn_timer_id_);
  this->syn_timer_id_ = -1; // invalidate handle
}

void
ReliableMulticast::leave_i()
{
  cancel_syn_timer();
}

} // namespace DCPS
} // namespace OpenDDS
