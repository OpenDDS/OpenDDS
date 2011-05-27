/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReliableSession.h"

#include "MulticastDataLink.h"
#include "MulticastConfiguration.h"

#include "ace/Global_Macros.h"
#include "ace/Time_Value.h"

#include "dds/DCPS/Serializer.h"

#include <cmath>
#include <cstdlib>

namespace OpenDDS {
namespace DCPS {

SynWatchdog::SynWatchdog(ReliableSession* session)
  : session_(session),
    retries_(0)
{
}

ACE_Time_Value
SynWatchdog::next_interval()
{
  MulticastConfiguration* config = this->session_->link()->config();
  ACE_Time_Value interval(config->syn_interval_);

  // Apply exponential backoff based on number of retries:
  if (this->retries_ > 0) {
    interval *= std::pow(config->syn_backoff_, double(this->retries_));
  }
  ++this->retries_;

  return interval;
}

void
SynWatchdog::on_interval(const void* /*arg*/)
{
  // Initiate handshake by sending a MULTICAST_SYN control
  // sample to the assigned remote peer:
  this->session_->send_syn();
}

ACE_Time_Value
SynWatchdog::next_timeout()
{
  MulticastConfiguration* config = this->session_->link()->config();
  return config->syn_timeout_;
}

void
SynWatchdog::on_timeout(const void* /*arg*/)
{
  // There is no recourse if a link is unable to handshake;
  // log an error and return:
  ACE_ERROR((LM_WARNING,
             ACE_TEXT("(%P|%t) WARNING: ")
             ACE_TEXT("SynWatchdog::on_timeout: ")
             ACE_TEXT("timed out waiting on remote peer: 0x%x!\n"),
             this->session_->remote_peer()));
}

NakWatchdog::NakWatchdog(ReliableSession* session)
  : session_(session)
{
}

ACE_Time_Value
NakWatchdog::next_interval()
{
  MulticastConfiguration* config = this->session_->link()->config();
  ACE_Time_Value interval(config->nak_interval_);

  // Apply random backoff to minimize potential collisions:
  interval *= static_cast<double>(std::rand()) /
              static_cast<double>(RAND_MAX) + 1.0;

  return interval;
}

void
NakWatchdog::on_interval(const void* /*arg*/)
{
  // Expire outstanding repair requests that have not yet been
  // fulfilled; this prevents NAK implosions due to remote
  // peers becoming unresponsive:
  this->session_->expire_naks();

  // Initiate repairs by sending MULTICAST_NAK control samples
  // to remote peers from which we are missing data:
  this->session_->send_naks();
}

ReliableSession::ReliableSession(MulticastDataLink* link,
                                 MulticastPeer remote_peer)
  : MulticastSession(link, remote_peer),
    acked_(false),
    syn_watchdog_(this),
    nak_watchdog_(this)
{
}

bool
ReliableSession::acked()
{
  ACE_READ_GUARD_RETURN(ACE_SYNCH_RW_MUTEX,
                        guard,
                        this->lock_,
                        false);

  return this->acked_;
}

bool
ReliableSession::header_received(const TransportHeader& header)
{
  // Update last seen sequence for remote peer; return false if we
  // have already seen this datagram to prevent duplicate delivery:
  return this->nak_sequence_.update(header.sequence_);
}

void
ReliableSession::control_received(char submessage_id,
                                  ACE_Message_Block* control)
{
  switch(submessage_id) {
    case MULTICAST_SYN:
      syn_received(control);
      break;

    case MULTICAST_SYNACK:
      synack_received(control);
      break;

    case MULTICAST_NAK:
      nak_received(control);
      break;

    case MULTICAST_NAKACK:
      nakack_received(control);
      break;

    default:
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ")
                 ACE_TEXT("ReliableSession::control_received: ")
                 ACE_TEXT("unknown TRANSPORT_CONTROL submessage: 0x%x!\n"),
                 submessage_id));
    }
}

void
ReliableSession::syn_received(ACE_Message_Block* control)
{
  ACE_WRITE_GUARD(ACE_SYNCH_RW_MUTEX,
                  guard,
                  this->lock_);
    
  // Obtain reference to received header:
  const TransportHeader& header =
    this->link_->receive_strategy()->received_header();

  TAO::DCPS::Serializer serializer(
    control, header.swap_bytes());

  MulticastPeer local_peer;
  serializer >> local_peer; // sent as remote_peer

  // Ignore sample if not destined for us:
  if (local_peer != this->link_->local_peer()) return;


  // Establish a baseline for detecting reception gaps:
  this->nak_sequence_.reset(header.sequence_);
  this->acked_ = true;

  // MULTICAST_SYN control samples are always positively
  // acknowledged by a matching remote peer:
  send_synack();
}

void
ReliableSession::send_syn()
{
  size_t len = sizeof(this->remote_peer_);

  ACE_Message_Block* data;
  ACE_NEW(data, ACE_Message_Block(len));

  TAO::DCPS::Serializer serializer(
    data, this->link_->transport()->swap_bytes());

  serializer << this->remote_peer_;

  // Send control sample to remote peer:
  send_control(MULTICAST_SYN, data);
}

void
ReliableSession::synack_received(ACE_Message_Block* control)
{
  {
    ACE_WRITE_GUARD(ACE_SYNCH_RW_MUTEX,
                    guard,
                    this->lock_);

    if (! this->acked_) {

      // Obtain reference to received header:
      const TransportHeader& header =
        this->link_->receive_strategy()->received_header();

      TAO::DCPS::Serializer serializer(
        control, header.swap_bytes());

      MulticastPeer local_peer;
      serializer >> local_peer; // sent as remote_peer

      // Ignore sample if not destined for us:
      if (local_peer != this->link_->local_peer()) return;

      this->syn_watchdog_.cancel();
      this->acked_ = true;
    }
  }
  
  // Force the TransportImpl to re-evaluate pending associations even
  // acked already to avoid some race causes datawriter never been notify
  // fully associated.
  MulticastTransport* transport = this->link_->transport();
  transport->check_fully_association();
}



void
ReliableSession::send_synack()
{
  size_t len = sizeof(this->remote_peer_);

  ACE_Message_Block* data;
  ACE_NEW(data, ACE_Message_Block(len));

  TAO::DCPS::Serializer serializer(
    data, this->link_->transport()->swap_bytes());

  serializer << this->remote_peer_;

  // Send control sample to remote peer:
  send_control(MULTICAST_SYNACK, data);
}

void
ReliableSession::expire_naks()
{
  if (this->nak_requests_.empty()) return; // nothing to expire

  ACE_Time_Value deadline(ACE_OS::gettimeofday());
  deadline -= this->link_->config()->nak_timeout_;

  NakRequestMap::iterator first(this->nak_requests_.begin());
  NakRequestMap::iterator last(this->nak_requests_.upper_bound(deadline));

  if (first == last) return; // nothing to expire

  for (NakRequestMap::iterator it(first); it != last; ++it) {
    // Skip unrecoverable datagrams if needed; attempt to
    // re-establish a baseline to detect future reception gaps:
    if (it->second > this->nak_sequence_) {
      ACE_ERROR((LM_WARNING,
                 ACE_TEXT("(%P|%t) WARNING: ")
                 ACE_TEXT("ReliableSession::expire_naks: ")
                 ACE_TEXT("timed out waiting on remote peer: 0x%x!\n"),
                 this->remote_peer_));

      this->nak_sequence_.reset(it->second);
    }
  }

  // Clear expired repair requests:
  this->nak_requests_.erase(first, last);
}

void
ReliableSession::send_naks()
{
  if (!this->nak_sequence_.disjoint()) return;  // nothing to send

  ACE_Time_Value now(ACE_OS::gettimeofday());

  // Record high-water mark for this interval; this value will
  // be used to reset the low-water mark in the event the remote
  // peer becomes unresponsive:
  this->nak_requests_.insert(
    NakRequestMap::value_type(now, this->nak_sequence_.high()));

  // Take a copy to facilitate temporary suppression:
  DisjointSequence missing(this->nak_sequence_);

  for (NakPeerSet::iterator it(this->nak_peers_.begin());
       it != this->nak_peers_.end(); ++it) {
    // Update sequence to temporarily suppress repair requests for
    // ranges already requested by other peers for this interval:
    missing.update(*it);
  }

  for (DisjointSequence::range_iterator range(missing.range_begin());
       range != missing.range_end(); ++range) {
    // Send MULTICAST_NAK control samples to remote peer; the
    // peer should respond with a resend of the missing data or
    // a MULTICAST_NAKACK indicating the data is unrecoverable:
    send_nak(range->first, range->second);
  }

  // Clear peer repair requests:
  this->nak_peers_.clear();
}

void
ReliableSession::nak_received(ACE_Message_Block* control)
{
  // Obtain reference to received header:
  const TransportHeader& header =
    this->link_->receive_strategy()->received_header();

  TAO::DCPS::Serializer serializer(
    control, header.swap_bytes());

  MulticastPeer local_peer;
  serializer >> local_peer; // sent as remote_peer

  MulticastSequence low;
  serializer >> low;

  MulticastSequence high;
  serializer >> high;

  SequenceRange range(low, high);

  // Track peer repair requests for later suppression:
  if (local_peer == this->remote_peer_) this->nak_peers_.insert(range);

  // Ignore sample if not destined for us:
  if (local_peer != this->link_->local_peer()) return;

  TransportSendBuffer* send_buffer = this->link_->send_buffer();
  if (!send_buffer->resend(range)) {
    // Broadcast a MULTICAST_NAKACK control sample to suppress
    // repair requests for unrecoverable samples by providing a
    // new low-water mark for affected peers:
    send_nakack(send_buffer->low());
  }
}

void
ReliableSession::send_nak(MulticastSequence low,
                          MulticastSequence high)
{
  size_t len = sizeof(this->remote_peer_)
             + sizeof(low)
             + sizeof(high);

  ACE_Message_Block* data;
  ACE_NEW(data, ACE_Message_Block(len));

  TAO::DCPS::Serializer serializer(
    data, this->link_->transport()->swap_bytes());

  serializer << this->remote_peer_;
  serializer << low;
  serializer << high;

  // Send control sample to remote peer:
  send_control(MULTICAST_NAK, data);
}

void
ReliableSession::nakack_received(ACE_Message_Block* control)
{
  // Obtain reference to received header:
  const TransportHeader& header =
    this->link_->receive_strategy()->received_header();

  // Ignore sample if remote peer not known:
  if (this->remote_peer_ != header.source_) return; // unknown peer

  TAO::DCPS::Serializer serializer(
    control, header.swap_bytes());

  MulticastSequence low;
  serializer >> low;

  // MULTICAST_NAKACK control samples indicate data which cannot be
  // repaired by a remote peer; update sequence to suppress repairs
  // by shifting to a new low-water mark if needed:
  if (!this->nak_sequence_.seen(low)) {
    ACE_ERROR((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: ")
               ACE_TEXT("ReliableSession::nakack_received: ")
               ACE_TEXT("unrecoverable samples reported by remote peer: 0x%x!\n"),
               this->remote_peer_));

    this->nak_sequence_.shift(low);
  }
}

void
ReliableSession::send_nakack(MulticastSequence low)
{
  size_t len = sizeof(low);

  ACE_Message_Block* data;
  ACE_NEW(data, ACE_Message_Block(len));

  TAO::DCPS::Serializer serializer(
    data, this->link_->transport()->swap_bytes());

  serializer << low;

  // Broadcast control sample to all peers:
  send_control(MULTICAST_NAKACK, data);
}

bool
ReliableSession::start(bool active)
{
  ACE_Reactor* reactor = this->link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ReliableSession::start: ")
                      ACE_TEXT("NULL reactor reference!\n")),
                     false);
  }

  // A watchdog timer is scheduled to periodically check for gaps in
  // received data. If a gap is discovered, MULTICAST_NAK control
  // samples will be sent to initiate repairs.
  if (!active && !this->nak_watchdog_.schedule(reactor)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ReliableSession::start: ")
                      ACE_TEXT("failed to schedule NAK watchdog!\n")),
                     false);
  }

  // Active peers schedule a watchdog timer to initiate a 2-way
  // handshake to verify that passive endpoints can send/receive
  // data reliably. This process must be executed using the
  // transport reactor thread to prevent blocking.
  if (active && !this->syn_watchdog_.schedule_now(reactor)) {
    this->nak_watchdog_.cancel();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ReliableSession::start: ")
                      ACE_TEXT("failed to schedule SYN watchdog!\n")),
                     false);
  }

  return true;
}

void
ReliableSession::stop()
{
  this->syn_watchdog_.cancel();
  this->nak_watchdog_.cancel();
}

} // namespace DCPS
} // namespace OpenDDS
