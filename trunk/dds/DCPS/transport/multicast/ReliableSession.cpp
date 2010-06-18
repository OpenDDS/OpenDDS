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
#include "ace/Truncate.h"

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
    started_(false),
    active_(true),
    syn_watchdog_(this),
    nak_watchdog_(this)
{
}

bool
ReliableSession::acked()
{
  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX,
                   guard,
                   this->ack_lock_,
                   false);

  return this->acked_;
}

bool
ReliableSession::check_header(const TransportHeader& header)
{
  // Not from the remote peer for this session.
  if (this->remote_peer_ != header.source_) return true;

  // Update last seen sequence for remote peer; return false if we
  // have already seen this datagram to prevent duplicate delivery:
  return this->nak_sequence_.update(header.sequence_);
}

void
ReliableSession::control_received(char submessage_id,
                                  ACE_Message_Block* control)
{
  // Record that we've gotten this message so we don't nak for it later.
  if (!this->acked_) {
    const TransportHeader& header = this->link_->receive_strategy()->received_header();
    if (this->remote_peer_ == header.source_) { 
      check_header(header);
    }
  }

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
  if (this->active_) return; // pub send syn, then doesn't receive them.

  const TransportHeader& header =
    this->link_->receive_strategy()->received_header();

  // Not from the remote peer for this session.
  if (this->remote_peer_ != header.source_) return;
 
  TAO::DCPS::Serializer serializer(
    control, header.swap_bytes());

  MulticastPeer local_peer;
  serializer >> local_peer; // sent as remote_peer

  // Ignore sample if not destined for us:
  if (local_peer != this->link_->local_peer()) return;

  {
    ACE_GUARD(ACE_SYNCH_MUTEX,
              guard,
              this->ack_lock_);

    if (!this->acked_) {
      this->acked_ = true;

      // Establish a baseline for detecting reception gaps:
      this->nak_sequence_.reset(header.sequence_);
    }
  }

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
  if (! this->active_) return; // sub send syn, then doesn't receive them.

  // Already received ack.
  if (this->acked_) return;

  const TransportHeader& header =
    this->link_->receive_strategy()->received_header();

  // Not from the remote peer for this session.
  if (this->remote_peer_ != header.source_) return;

  TAO::DCPS::Serializer serializer(
    control, header.swap_bytes());

  MulticastPeer local_peer;
  serializer >> local_peer; // sent as remote_peer

  // Ignore sample if not destined for us:
  if (local_peer != this->link_->local_peer()) return;

  {
    ACE_GUARD(ACE_SYNCH_MUTEX,
              guard,
              this->ack_lock_);

    if (this->acked_) return; // already acked

    this->syn_watchdog_.cancel();
    this->acked_ = true;
  }
 
  // Force the TransportImpl to re-evaluate pending associations
  // after deliver synack to every session.
  this->link_->set_check_fully_association();
}



void
ReliableSession::send_synack()
{
  // Send nakack before sending synack to 
  // reduce naks from remote.
  TransportSendBuffer* send_buffer = this->link_->send_buffer();
  if (!send_buffer->empty() && send_buffer->low() > ++SequenceNumber()) {
    send_nakack(send_buffer->low()); 
  }

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
  // Could get data samples before syn control message.
  // No use nak'ing until syn control message is received and session is acked.
  if (!this->acked_) return; 

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

  send_naks (missing);
  
  // Clear peer repair requests:
  this->nak_peers_.clear();
}

void
ReliableSession::nak_received(ACE_Message_Block* control)
{  
  if (! this->active_) return; // sub send naks, then doesn't receive them.

  const TransportHeader& header =
    this->link_->receive_strategy()->received_header();

  TAO::DCPS::Serializer serializer(
    control, header.swap_bytes());

  MulticastPeer local_peer;
  CORBA::ULong size = 0;
  serializer >> local_peer; // sent as remote_peer
  serializer >> size; 

  std::vector<SequenceRange> ranges;

  for (CORBA::ULong i = 0; i < size; ++i) {
    MulticastSequence low;
    serializer >> low;

    MulticastSequence high;
    serializer >> high;

    ranges.push_back (SequenceRange (low, high));
  }
  
  // Track peer repair requests for later suppression:
  if (local_peer == this->remote_peer_) { 
    for (CORBA::ULong i = 0; i < size; ++i) {
      this->nak_peers_.insert(ranges[i]); 
    }
    return;
  }

  // Ignore sample if not destined for us:
  if ((local_peer != this->link_->local_peer())        // Not to us.
    || (this->remote_peer_ != header.source_)) return; // Not from the remote peer for this session. 

  TransportSendBuffer* send_buffer = this->link_->send_buffer();
  // Broadcast a MULTICAST_NAKACK control sample before resending to suppress
  // repair requests for unrecoverable samples by providing a
  // new low-water mark for affected peers:
  if (!send_buffer->empty() && send_buffer->low() > ranges.begin()->first) {
    send_nakack(send_buffer->low()); 
  }

  for (CORBA::ULong i = 0; i < size; ++i) {
    bool ret = send_buffer->resend(ranges[i]);
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t)ReliableSession::nak_received")
                            ACE_TEXT (" %d <- %d %d - %d resend result %d\n"),
                            this->link_->local_peer(), this->remote_peer_, 
                            ranges[i].first.value_, ranges[i].second.value_, ret));  
    }
  }
}

void
ReliableSession::send_naks (DisjointSequence& missing)
{  
  std::vector<DisjointSequence::range_iterator> ranges;
  
  for (DisjointSequence::range_iterator range(missing.range_begin());
       range != missing.range_end(); ++range) {
    // Send MULTICAST_NAK control samples to remote peer; the
    // peer should respond with a resend of the missing data or
    // a MULTICAST_NAKACK indicating the data is unrecoverable:
    ranges.push_back (range);
  }
  
  CORBA::ULong size = ACE_Utils::truncate_cast<CORBA::ULong> (ranges.size());

  size_t len = sizeof(this->remote_peer_)
             + sizeof(size)
             + size * 2 * sizeof(SequenceNumber);
             
  ACE_Message_Block* data;
  ACE_NEW(data, ACE_Message_Block(len));

  TAO::DCPS::Serializer serializer(
    data, this->link_->transport()->swap_bytes());

  serializer << this->remote_peer_;
  serializer << size;
  for (CORBA::ULong i = 0; i < size; ++i) {
    serializer << ranges[i]->first;
    serializer << ranges[i]->second;
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t)ReliableSession::send_naks")
                            ACE_TEXT (" %d -> %d %d - %d \n"), 
                            this->link_->local_peer(), remote_peer_,
                            ranges[i]->first.value_, ranges[i]->second.value_));
    }
  }
  // Send control sample to remote peer:
  send_control(MULTICAST_NAK, data);
}


void
ReliableSession::nakack_received(ACE_Message_Block* control)
{
  if (this->active_) return; // pub send syn, then doesn't receive them.

  const TransportHeader& header =
    this->link_->receive_strategy()->received_header();

  // Not from the remote peer for this session.
  if (this->remote_peer_ != header.source_) return; 

  TAO::DCPS::Serializer serializer(
    control, header.swap_bytes());

  MulticastSequence low;
  serializer >> low;

  // MULTICAST_NAKACK control samples indicate data which cannot be
  // repaired by a remote peer; update sequence to suppress repairs
  // by shifting to a new low-water mark if needed:
  if (!this->nak_sequence_.seen(low)) {
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_ERROR((LM_WARNING,
                ACE_TEXT("(%P|%t) WARNING: ")
                ACE_TEXT("ReliableSession::nakack_received %d <- %d %d - %d ")
                ACE_TEXT("not repaired.\n"),
                this->link_->local_peer(), this->remote_peer_, 
                this->nak_sequence_.low().value_, low));
    }
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
  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX,
                   guard,
                   this->start_lock_,
                   false);

  if (this->started_) return true;  // already started

  ACE_Reactor* reactor = this->link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ReliableSession::start: ")
                      ACE_TEXT("NULL reactor reference!\n")),
                     false);
  }

  this->active_  = active;
  // A watchdog timer is scheduled to periodically check for gaps in
  // received data. If a gap is discovered, MULTICAST_NAK control
  // samples will be sent to initiate repairs.
  // Only subscriber send naks so just schedule for sub role.
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
  // Only publisher send syn so just schedule for pub role.
  if (active && !this->syn_watchdog_.schedule_now(reactor)) {
    this->nak_watchdog_.cancel();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("ReliableSession::start: ")
                      ACE_TEXT("failed to schedule SYN watchdog!\n")),
                     false);
  }

  return this->started_ = true;
}

void
ReliableSession::stop()
{
  this->syn_watchdog_.cancel();
  this->nak_watchdog_.cancel();
}

} // namespace DCPS
} // namespace OpenDDS
