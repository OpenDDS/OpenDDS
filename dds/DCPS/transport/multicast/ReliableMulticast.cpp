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
#include "dds/DCPS/transport/framework/TransportSendBuffer.h"

#include <cmath>

namespace OpenDDS {
namespace DCPS {

SynWatchdog::SynWatchdog(ReliableMulticast* link)
  : DataLinkWatchdog<ReliableMulticast>(link),
    retries_(0)
{
}

ACE_Time_Value
SynWatchdog::next_interval()
{
  MulticastConfiguration* config = this->link_->config();

  ACE_Time_Value interval(config->syn_interval_);
  if (this->retries_ > 0) {
    // Apply exponential backoff based on number of retries:
    interval *= std::pow(config->syn_backoff_, this->retries_);
  }
  ++this->retries_;

  return interval;
}

void
SynWatchdog::on_interval(const void* /*arg*/)
{
  // Initiate handshake by sending a MULTICAST_SYN control
  // sample to the assigned remote peer:
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
  // There is no recourse if a link is unable to handshake;
  // log an error and return:
  ACE_ERROR((LM_ERROR,
             ACE_TEXT("(%P|%t) ERROR: ")
             ACE_TEXT("SynWatchdog::on_timeout: ")
             ACE_TEXT("timed out waiting on remote peer: 0x%x!\n"),
             this->link_->remote_peer()));
}

NakWatchdog::NakWatchdog(ReliableMulticast* link)
  : DataLinkWatchdog<ReliableMulticast>(link)
{
}

ACE_Time_Value
NakWatchdog::next_interval()
{
  MulticastConfiguration* config = this->link_->config();

  ACE_Time_Value interval(config->nak_interval_);
  // Apply random backoff to minimize potential collisions:
  interval *= (++this->random_ + 1.0);

  return interval;
}

void
NakWatchdog::on_interval(const void* /*arg*/)
{
  // Expire outstanding repair requests that have not yet been
  // fulfilled; this prevents NAK implosions due to remote
  // peers becoming unresponsive:
  this->link_->expire_naks();

  // Initiate repairs by sending MULTICAST_NAK control samples
  // to remote peers from which we are missing data:
  this->link_->send_naks();
}

ReliableMulticast::ReliableMulticast(MulticastTransport* transport,
                                     MulticastPeer local_peer,
                                     MulticastPeer remote_peer,
                                     bool active)
  : MulticastDataLink(transport,
                      local_peer,
                      remote_peer,
                      active),
    acked_(false),
    syn_watchdog_(this),
    nak_watchdog_(this)
{
}

ReliableMulticast::~ReliableMulticast()
{
  if (!this->send_buffer_.is_nil()) {
    this->send_strategy_->send_buffer(0);

    this->send_buffer_->_remove_ref();  // release ownership
    this->send_buffer_ = 0;
  }
}

void
ReliableMulticast::expire_naks()
{
  if (this->nak_requests_.empty()) return; // nothing to expire

  ACE_Time_Value deadline(ACE_OS::gettimeofday());
  deadline -= this->config_->nak_timeout_;

  NakRequestMap::iterator first(this->nak_requests_.begin());
  NakRequestMap::iterator last(this->nak_requests_.upper_bound(deadline));

  if (first == last) return; // nothing to expire

  for (NakRequestMap::iterator it(first); it != last; ++it) {
    NakRequest& request(it->second);

    NakSequenceMap::iterator sequence(this->nak_sequences_.find(request.first));
    if (sequence == this->nak_sequences_.end()) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("ReliableMulticast::expire_naks: ")
                 ACE_TEXT("failed to find sequence for remote peer: 0x%x!\n"),
                 request.first));
      continue;
    }

    // Skip unrecoverable datagrams if needed; attempt to
    // re-establish a baseline to detect future reception gaps:
    if (request.second > sequence->second) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("ReliableMulticast::expire_naks: ")
                 ACE_TEXT("timed out waiting on remote peer: 0x%x!\n"),
                 request.first));

      sequence->second.skip(request.second);
    }
  }

  // Clear expired repair requests:
  this->nak_requests_.erase(first, last);
}

void
ReliableMulticast::send_naks()
{
  for (NakSequenceMap::iterator it(this->nak_sequences_.begin());
       it != this->nak_sequences_.end(); ++it) {

    if (!it->second.disjoint()) continue; // nothing to send

    ACE_Time_Value now(ACE_OS::gettimeofday());

    // Record high-water mark for peer on this interval; this value
    // is used to reset the low-water mark in the event the peer
    // becomes unresponsive:
    this->nak_requests_.insert(NakRequestMap::value_type(
      now, NakRequest(it->first, it->second.high())));

    // Take a copy to facilitate temporary suppression:
    DisjointSequence missing(it->second);

    for (NakPeerMap::iterator peer(this->nak_peers_.find(it->first));
         peer != this->nak_peers_.end(); ++peer) {
      // Update set to temporarily suppress repair requests for
      // ranges already requested by other peers on this interval:
      missing.update(peer->second);
    }

    for (DisjointSequence::range_iterator range(missing.range_begin());
         range != missing.range_end(); ++range) {
      // Send MULTICAST_NAK control samples to remote peer; the
      // peer should respond with a resend of the missing data or
      // a MULTICAST_NAKACK indicating the data is unrecoverable:
      send_nak(it->first, range->first, range->second);
    }
  }

  // Clear peer repair requests:
  this->nak_peers_.clear();
}

bool
ReliableMulticast::acked()
{
  return this->acked_;
}

bool
ReliableMulticast::header_received(const TransportHeader& header)
{
  this->received_header_ = header;

  NakSequenceMap::iterator it(this->nak_sequences_.find(header.source_));
  if (it == this->nak_sequences_.end()) return true;  // unknown peer

  // Update last seen sequence for remote peer; return false if we
  // have already seen this datagram to prevent duplicate delivery:
  return it->second.update(header.sequence_);
}

void
ReliableMulticast::sample_received(ReceivedDataSample& sample)
{
  switch(sample.header_.message_id_) {
  case TRANSPORT_CONTROL:
    switch (sample.header_.submessage_id_) {
    case MULTICAST_SYN:
      syn_received(sample.sample_);
      break;

    case MULTICAST_SYNACK:
      synack_received(sample.sample_);
      break;

    case MULTICAST_NAK:
      nak_received(sample.sample_);
      break;

    case MULTICAST_NAKACK:
      nakack_received(sample.sample_);
      break;

    default:
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ")
                 ACE_TEXT("ReliableMulticast::sample_received: ")
                 ACE_TEXT("unknown TRANSPORT_CONTROL submessage: 0x%x!\n"),
                 sample.header_.submessage_id_));
    }
    break;

  case SAMPLE_ACK:
    ack_received(sample);
    break;

  default:
    data_received(sample);
  }
}

void
ReliableMulticast::syn_received(ACE_Message_Block* control)
{
  TAO::DCPS::Serializer serializer(
    control, this->transport_->swap_bytes());

  MulticastPeer local_peer;
  serializer >> local_peer; // sent as remote_peer

  // Ignore sample if not destined for us:
  if (local_peer != this->local_peer_) return;

  // Fetch remote peer from header:
  MulticastPeer remote_peer(this->received_header_.source_);

  // Insert remote peer into sequence map; this establishes a
  // baseline for detecting reception gaps during delivery:
  this->nak_sequences_.insert(NakSequenceMap::value_type(
    remote_peer, DisjointSequence(this->received_header_.sequence_)));

  // MULTICAST_SYN control samples are always positively
  // acknowledged by a matching remote peer:
  send_synack(remote_peer);
}

void
ReliableMulticast::send_syn()
{
  size_t len = sizeof(this->remote_peer_);

  ACE_Message_Block* data;
  ACE_NEW(data, ACE_Message_Block(len));

  TAO::DCPS::Serializer serializer(
    data, this->transport_->swap_bytes());

  serializer << this->remote_peer_;

  // Send control sample to remote peer:
  send_control(MULTICAST_SYN, data);
}

void
ReliableMulticast::synack_received(ACE_Message_Block* control)
{
  // Ignore sample if already acked:
  if (this->acked_) return;

  TAO::DCPS::Serializer serializer(
    control, this->transport_->swap_bytes());

  MulticastPeer local_peer;
  serializer >> local_peer; // sent as remote_peer

  // Ignore sample if not destined for us:
  if (local_peer != this->local_peer_) return;

  this->syn_watchdog_.cancel();

  // Handshake is complete; adjust the acked flag and force the
  // TransportImpl to re-evaluate any pending associations:
  this->acked_ = true;
  this->transport_->check_fully_association();
}

void
ReliableMulticast::send_synack(MulticastPeer remote_peer)
{
  size_t len = sizeof(remote_peer);

  ACE_Message_Block* data;
  ACE_NEW(data, ACE_Message_Block(len));

  TAO::DCPS::Serializer serializer(
    data, this->transport_->swap_bytes());

  serializer << remote_peer;

  // Send control sample to remote peer:
  send_control(MULTICAST_SYNACK, data);
}

void
ReliableMulticast::nak_received(ACE_Message_Block* control)
{
  TAO::DCPS::Serializer serializer(
    control, this->transport_->swap_bytes());

  MulticastPeer local_peer;
  serializer >> local_peer; // sent as remote_peer

  MulticastSequence low;
  serializer >> low;

  MulticastSequence high;
  serializer >> high;

  SequenceRange range(low, high);

  // Record repair request for known peer:
  if (this->nak_sequences_.find(local_peer) != this->nak_sequences_.end()) {
    this->nak_peers_.insert(NakPeerMap::value_type(local_peer, range));
  }

  // Ignore sample if not destined for us:
  if (local_peer != this->local_peer_) return;

  if (!this->send_buffer_->resend(range)) {
    // Broadcast a MULTICAST_NAKACK control sample to suppress
    // repair requests for unrecoverable samples by providing a
    // new low-water mark for affected peers:
    send_nakack(this->send_buffer_->low());
  }
}

void
ReliableMulticast::send_nak(MulticastPeer remote_peer,
                            MulticastSequence low,
                            MulticastSequence high)
{
  size_t len = sizeof(remote_peer)
             + sizeof(low)
             + sizeof(high);

  ACE_Message_Block* data;
  ACE_NEW(data, ACE_Message_Block(len));

  TAO::DCPS::Serializer serializer(
    data, this->transport_->swap_bytes());

  serializer << remote_peer;
  serializer << low;
  serializer << high;

  // Send control sample to remote peer:
  send_control(MULTICAST_NAK, data);
}

void
ReliableMulticast::nakack_received(ACE_Message_Block* control)
{
  // Fetch remote peer from header:
  MulticastPeer remote_peer(this->received_header_.source_);

  // Ignore sample if remote peer not known:
  NakSequenceMap::iterator it(this->nak_sequences_.find(remote_peer));
  if (it == this->nak_sequences_.end()) return;

  TAO::DCPS::Serializer serializer(
    control, this->transport_->swap_bytes());

  MulticastSequence low;
  serializer >> low;

  // MULTICAST_NAKACK control samples indicate data which cannot be
  // repaired by a remote peer; update the sequence map if needed
  //  to suppress repairs by shifting to a new low-water mark:
  if (!it->second.seen(low)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("ReliableMulticast::nakack_received: ")
               ACE_TEXT("unrecoverable samples reported by remote peer: 0x%x!\n"),
               remote_peer));

    it->second.shift(low);
  }
}

void
ReliableMulticast::send_nakack(MulticastSequence low)
{
  size_t len = sizeof(low);

  ACE_Message_Block* data;
  ACE_NEW(data, ACE_Message_Block(len));

  TAO::DCPS::Serializer serializer(
    data, this->transport_->swap_bytes());

  serializer << low;

  // Broadcast control sample to remote peers:
  send_control(MULTICAST_NAKACK, data);
}

void
ReliableMulticast::send_control(SubMessageId submessage_id,
                                ACE_Message_Block* data)
{
  ACE_Message_Block* control = create_control(submessage_id, data);
  if (control == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("ReliableMulticast::send_control: ")
               ACE_TEXT("create_control failed!\n")));
    return;
  }

  int error;
  if ((error = DataLink::send_control(control)) != SEND_CONTROL_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("ReliableMulticast::send_control: ")
               ACE_TEXT("send_control failed: %d!\n"),
               error));
    return;
  }
}

void
ReliableMulticast::send_strategy(MulticastSendStrategy* send_strategy)
{
  // A send buffer is bound to the send strategy to ensure a
  // configured number of most-recent datagrams are retained in
  // order to fulfill repair requests:
  this->send_buffer_ =
    new TransportSendBuffer(this->config_->nak_depth_,
                            this->config_->max_samples_per_packet_);
  if (this->send_buffer_.is_nil()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("ReliableMulticast::send_strategy: ")
               ACE_TEXT("failed to create TransportSendBuffer!\n")));
    return;
  }
  this->send_buffer_->_add_ref(); // take ownership

  send_strategy->send_buffer(this->send_buffer_.in());

  MulticastDataLink::send_strategy(send_strategy);  // delegate to parent
}

int
ReliableMulticast::start_i()
{
  ACE_Reactor* reactor = get_reactor();
  if (reactor == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ReliableMulticast::start_i: ")
                      ACE_TEXT("NULL reactor reference!\n")),
                     -1);
  }

  // A watchdog timer is scheduled to periodically check for gaps in
  // received data. If a gap is discovered, MULTICAST_NAK control
  // samples will be sent to initiate repairs.
  if (!this->nak_watchdog_.schedule(reactor)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ReliableMulticast::start_i: ")
                      ACE_TEXT("failed to schedule NAK watchdog!\n")),
                     -1);
  }

  // Active peers schedule a watchdog timer to initiate a 2-way
  // handshake to verify that passive endpoints can send/receive
  // data reliably. This process must be executed using the
  // transport reactor thread to prevent blocking.
  if (this->active_ && !this->syn_watchdog_.schedule_now(reactor)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ReliableMulticast::start_i: ")
                      ACE_TEXT("failed to schedule SYN watchdog!\n")),
                     -1);
  }

  return 0;
}

void
ReliableMulticast::stop_i()
{
  this->syn_watchdog_.cancel();
  this->nak_watchdog_.cancel();

  MulticastDataLink::stop_i();  // delegate to parent
}

} // namespace DCPS
} // namespace OpenDDS
