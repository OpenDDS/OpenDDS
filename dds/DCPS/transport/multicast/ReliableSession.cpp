/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReliableSession.h"

#include "MulticastDataLink.h"
#include "MulticastInst.h"
#include "MulticastReceiveStrategy.h"

#include "ace/Global_Macros.h"
#include "ace/Time_Value.h"
#include "ace/Truncate.h"

#include "dds/DCPS/Serializer.h"
#include <cstdlib>

namespace OpenDDS {
namespace DCPS {

NakWatchdog::NakWatchdog(ACE_Reactor* reactor,
                         ACE_thread_t owner,
                         ReliableSession* session)
  : DataLinkWatchdog(reactor, owner)
  , session_(session)
{
}

ACE_Time_Value
NakWatchdog::next_interval()
{
  MulticastInst* config = this->session_->link()->config();
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

ReliableSession::ReliableSession(ACE_Reactor* reactor,
                                 ACE_thread_t owner,
                                 MulticastDataLink* link,
                                 MulticastPeer remote_peer)
  : MulticastSession(reactor, owner, link, remote_peer),
    nak_watchdog_(new NakWatchdog (reactor, owner, this))
{
}

ReliableSession::~ReliableSession()
{
  nak_watchdog_->cancel();
  nak_watchdog_->wait();
  nak_watchdog_->destroy();
}

bool
NakWatchdog::reactor_is_shut_down() const
{
  return session_->link()->transport()->is_shut_down();
}

bool
ReliableSession::check_header(const TransportHeader& header)
{
  // Not from the remote peer for this session.
  if (this->remote_peer_ != header.source_) return true;

  // Update last seen sequence for remote peer; return false if we
  // have already seen this datagram to prevent duplicate delivery:
  return this->nak_sequence_.insert(header.sequence_);
}

bool
ReliableSession::control_received(char submessage_id,
                                  ACE_Message_Block* control)
{
  if (MulticastSession::control_received(submessage_id, control)) {
    return true; // base class handled message
  }

  switch (submessage_id) {
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
    break;
  }
  return true;
}

void
ReliableSession::syn_hook(const SequenceNumber& seq)
{
  // Establish a baseline for detecting reception gaps:
  this->nak_sequence_.reset();
  this->nak_sequence_.insert(SequenceRange(SequenceNumber(), seq));
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

  // Skip unrecoverable datagrams to
  // re-establish a baseline to detect future reception gaps.
  SequenceNumber lastSeq = (last == this->nak_requests_.end())
                         ? this->nak_requests_.rbegin()->second
                         : last->second;

  std::vector<SequenceRange> dropped;
  if (this->nak_sequence_.insert(SequenceRange(this->nak_sequence_.low(),
                                               lastSeq), dropped)) {

    for (size_t i = 0; i < dropped.size(); ++i) {
      this->reassembly_.data_unavailable(dropped[i]);
    }

    ACE_ERROR((LM_WARNING,
                ACE_TEXT("(%P|%t) WARNING: ")
                ACE_TEXT("ReliableSession::expire_naks: ")
                ACE_TEXT("timed out waiting on remote peer %d to send missing samples: %q - %q!\n"),
                this->remote_peer_, this->nak_sequence_.low().getValue(), lastSeq.getValue()));
  }

  // Clear expired repair requests:
  this->nak_requests_.erase(first, last);
}

void
ReliableSession::send_naks()
{
  // Could get data samples before syn control message.
  // No use nak'ing until syn control message is received and session is acked.
  if (!this->acked()) {
    if (DCPS_debug_level > 5) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) ReliableSession::send_naks local %d ")
                           ACE_TEXT("remote %d session NOT acked yet, don't send naks\n"),
        this->link_->local_peer(), this->remote_peer_));
    }
    return;
  }

  if (DCPS_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) ReliableSession::send_naks local %d ")
                         ACE_TEXT("remote %d nak request size %d \n"),
      this->link_->local_peer(), this->remote_peer_, this->nak_requests_.size()));
  }

  if (!(this->nak_sequence_.low() > 1) && !this->nak_sequence_.disjoint()) {
    if (DCPS_debug_level > 5) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) ReliableSession::send_naks local %d ")
                           ACE_TEXT("remote %d nak sequence not disjoint, don't send naks \n"),
        this->link_->local_peer(), this->remote_peer_));
    }
    return;  // nothing to send
  }

  ACE_Time_Value now(ACE_OS::gettimeofday());

  // Record low-water mark for this interval; this value will
  // be used to reset the low-water mark in the event the remote
  // peer becomes unresponsive:
  if (this->nak_sequence_.low() > 1) {
    this->nak_requests_[now] = SequenceNumber();
  } else {
    this->nak_requests_[now] = this->nak_sequence_.cumulative_ack();
  }

  typedef std::vector<SequenceRange> RangeVector;
  RangeVector ignored;

  /// The range first - second will be skipped (no naks sent for it).
  SequenceNumber first;
  SequenceNumber second;

  NakRequestMap::reverse_iterator itr(this->nak_requests_.rbegin());

  if (this->nak_requests_.size() > 1) {
    // The sequences between rbegin - 1 and rbegin will not be ignored for naking.
    ++itr;

    size_t nak_delay_intervals = this->link()->config()->nak_delay_intervals_;
    size_t nak_max = this->link()->config()->nak_max_;
    size_t sz = this->nak_requests_.size();

    // Image i is the index of element in nak_requests_ in reverse order.
    // index 0 sequence is most recent high water mark.
    // e.g index , 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
    //  0 (rbegin) is always skipped because missing sample between 1 and 0 interval
    //  should always be naked.,
    //  if nak_delay_intervals=4, nak_max=3, any sequence between 5 - 1, 10 - 6, 15 - 11
    //  are skipped for naking due to nak_delay_intervals and 20 - 16 are skipped for
    //  naking due to nak_max.
    for (size_t i = 1; i < sz; ++i) {
      if ((i * 1.0) / (nak_delay_intervals + 1) > nak_max) {
        if (first != SequenceNumber()) {
          first = this->nak_requests_.begin()->second;
        }
        else {
          ignored.push_back(std::make_pair(this->nak_requests_.begin()->second, itr->second));
        }
        break;
      }

      if (i % (nak_delay_intervals + 1) == 1) {
        second = itr->second;
      }
      if (second != SequenceNumber()) {
        first = itr->second;
      }

      if (i % (nak_delay_intervals + 1) == 0) {
        first = itr->second;

        if (first != SequenceNumber() && second != SequenceNumber()) {
          ignored.push_back(std::make_pair(first, second));
          first = SequenceNumber();
          second = SequenceNumber();
        }
      }

      ++itr;
    }

    if (first != SequenceNumber() && second != SequenceNumber() && first != second) {
      ignored.push_back(std::make_pair(first, second));
    }
  }

  // Take a copy to facilitate temporary suppression:
  DisjointSequence received(this->nak_sequence_);
  if (DCPS_debug_level > 0) {
    received.dump();
  }

  size_t sz = ignored.size();
  for (size_t i = 0; i < sz; ++i) {

    if (ignored[i].second > received.cumulative_ack()) {
      SequenceNumber high = ignored[i].second;
      SequenceNumber low = ignored[i].first;
      if (low < received.cumulative_ack()) {
        low = received.cumulative_ack();
      }

      if (DCPS_debug_level > 5) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) ReliableSession::send_naks local %d ")
          ACE_TEXT("remote %d ignore missing [%q - %q]\n"),
          this->link_->local_peer(), this->remote_peer_, low.getValue(), high.getValue()));
      }

      // Make contiguous between ignored sequences.
      received.insert(SequenceRange(low, high));
    }
  }

  for (NakPeerSet::iterator it(this->nak_peers_.begin());
       it != this->nak_peers_.end(); ++it) {
    // Update sequence to temporarily suppress repair requests for
    // ranges already requested by other peers for this interval:
    received.insert(*it);
  }
  bool sending_naks = false;
  if (received.low() > 1){
    //Special case: nak from beginning to make sure no missing sequence
    //number below the first received
    sending_naks = true;
    std::vector<SequenceRange> ranges;
    ranges.push_back(SequenceRange(SequenceNumber(), received.low()));

    CORBA::ULong size = ACE_Utils::truncate_cast<CORBA::ULong>(ranges.size());

    size_t len = sizeof(this->remote_peer_)
               + sizeof(size)
               + size * 2 * sizeof(SequenceNumber);

    ACE_Message_Block* data;
    ACE_NEW(data, ACE_Message_Block(len));

    Serializer serializer(data);

    serializer << this->remote_peer_;
    serializer << size;
    for (std::vector<SequenceRange>::const_iterator iter = ranges.begin();
         iter != ranges.end(); ++iter) {
      serializer << iter->first;
      serializer << iter->second;
      if (DCPS_debug_level > 0) {
        ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) ReliableSession::send_naks ")
                              ACE_TEXT (" local %d remote %d [%q - %q]\n"),
                              this->link_->local_peer(), remote_peer_,
                              iter->first.getValue(), iter->second.getValue()));
      }
    }
    // Send control sample to remote peer:
    send_control(MULTICAST_NAK, data);
  }
  if (received.disjoint()) {
    sending_naks = true;
    send_naks(received);
  }
  if (!sending_naks && DCPS_debug_level > 5){
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) ReliableSession::send_naks local %d ")
                         ACE_TEXT("remote %d received sequence not disjoint, don't send naks \n"),
      this->link_->local_peer(), this->remote_peer_));
  }

  // Clear peer repair requests:
  this->nak_peers_.clear();
}

void
ReliableSession::nak_received(ACE_Message_Block* control)
{
  if (!this->active_) return; // sub send naks, then doesn't receive them.

  const TransportHeader& header =
    this->link_->receive_strategy()->received_header();

  Serializer serializer(control, header.swap_bytes());

  MulticastPeer local_peer;
  CORBA::ULong size = 0;
  serializer >> local_peer; // sent as remote_peer
  serializer >> size;

  std::vector<SequenceRange> ranges;

  for (CORBA::ULong i = 0; i < size; ++i) {
    SequenceRange range;
    serializer >> range.first;
    serializer >> range.second;
    ranges.push_back(range);
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

  SingleSendBuffer* send_buffer = this->link_->send_buffer();
  // Broadcast a MULTICAST_NAKACK control sample before resending to suppress
  // repair requests for unrecoverable samples by providing a
  // new low-water mark for affected peers:
  if (!send_buffer->empty() && send_buffer->low() > ranges.begin()->first) {
    send_nakack(send_buffer->low());
  }

  for (CORBA::ULong i = 0; i < size; ++i) {
    bool ret = send_buffer->resend(ranges[i]);
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("(%P|%t) ReliableSession::nak_received")
                  ACE_TEXT (" local %d remote %d [%q - %q] resend result %C\n"),
                  this->link_->local_peer(), this->remote_peer_,
                  ranges[i].first.getValue(), ranges[i].second.getValue(),
                  ret ? "SUCCESS" : "FAILED"));
    }
  }
}

void
ReliableSession::send_naks(DisjointSequence& received)
{
  const std::vector<SequenceRange> ranges(received.missing_sequence_ranges());

  CORBA::ULong size = ACE_Utils::truncate_cast<CORBA::ULong>(ranges.size());

  size_t len = sizeof(this->remote_peer_)
             + sizeof(size)
             + size * 2 * sizeof(SequenceNumber);

  ACE_Message_Block* data;
  ACE_NEW(data, ACE_Message_Block(len));

  Serializer serializer(data);

  serializer << this->remote_peer_;
  serializer << size;
  for (std::vector<SequenceRange>::const_iterator iter = ranges.begin();
       iter != ranges.end(); ++iter) {
    serializer << iter->first;
    serializer << iter->second;
    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("(%P|%t) ReliableSession::send_naks (Disjoint) ")
                            ACE_TEXT (" local %d remote %d [%q - %q]\n"),
                            this->link_->local_peer(), remote_peer_,
                            iter->first.getValue(), iter->second.getValue()));
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

  Serializer serializer(control, header.swap_bytes());

  SequenceNumber low;
  serializer >> low;

  // MULTICAST_NAKACK control samples indicate data which cannot be
  // repaired by a remote peer; if any values were needed below
  // this value, then the sequence needs to be shifted:
  std::vector<SequenceRange> dropped;
  SequenceNumber range_low = low.previous() < this->nak_sequence_.low() ? SequenceNumber() : low.previous();
  SequenceNumber range_high = low.previous() < this->nak_sequence_.low() ? low.previous() : this->nak_sequence_.low();

  if (range_low == SequenceNumber() && range_high == SequenceNumber()) {
    //clearout the nak_sequence_ so initial range can be inserted,
    //then re-insert ranges present before
    DisjointSequence tmp(this->nak_sequence_);
    this->nak_sequence_.reset();
    this->nak_sequence_.insert(SequenceRange(range_low, range_high));
    const std::vector<SequenceRange> ranges(tmp.present_sequence_ranges());

    for (std::vector<SequenceRange>::const_iterator iter = ranges.begin();
         iter != ranges.end(); ++iter) {
      this->nak_sequence_.insert(SequenceRange(iter->first, iter->second));
    }
  } else if (this->nak_sequence_.insert(SequenceRange(range_low, range_high), dropped)) {

    for (size_t i = 0; i < dropped.size(); ++i) {
      this->reassembly_.data_unavailable(dropped[i]);
    }

    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_WARNING,
                ACE_TEXT("(%P|%t) WARNING: ReliableSession::nakack_received ")
                ACE_TEXT("local %d remote %d [%q - %q] ")
                ACE_TEXT("not repaired.\n"),
                this->link_->local_peer(), this->remote_peer_,
                this->nak_sequence_.low().getValue(), low.getValue()));
    }
  }
}

void
ReliableSession::send_nakack(SequenceNumber low)
{
  size_t len = sizeof(low.getValue());

  ACE_Message_Block* data;
  ACE_NEW(data, ACE_Message_Block(len));

  Serializer serializer(data);

  serializer << low;
  // Broadcast control sample to all peers:
  send_control(MULTICAST_NAKACK, data);
}

bool
ReliableSession::start(bool active, bool acked)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, this->start_lock_, false);

  if (this->started_) {
    return true;  // already started
  }

  this->active_  = active;
  {
    //can't call accept_datalink while holding lock due to possible reactor deadlock with passive_connection
    ACE_GUARD_RETURN(Reverse_Lock_t, unlock_guard, this->reverse_start_lock_, false);

    // A watchdog timer is scheduled to periodically check for gaps in
    // received data. If a gap is discovered, MULTICAST_NAK control
    // samples will be sent to initiate repairs.
    // Only subscriber send naks so just schedule for sub role.
    if (!active) {
      if (acked) {
        this->set_acked();
      }
      if (!this->nak_watchdog_->schedule()) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: ")
                          ACE_TEXT("ReliableSession::start: ")
                          ACE_TEXT("failed to schedule NAK watchdog!\n")),
                         false);
      }
    }

    // Active peers schedule a watchdog timer to initiate a 2-way
    // handshake to verify that passive endpoints can send/receive
    // data reliably. This process must be executed using the
    // transport reactor thread to prevent blocking.
    // Only publisher send syn so just schedule for pub role.
    if (active && !this->start_syn()) {
      this->nak_watchdog_->cancel();
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("ReliableSession::start: ")
                        ACE_TEXT("failed to schedule SYN watchdog!\n")),
                       false);
    }
  } //Reacquire start_lock_ after releasing unlock_guard with release_start_lock_

  return this->started_ = true;
}

void
ReliableSession::stop()
{
  MulticastSession::stop();
  this->nak_watchdog_->cancel();
}

} // namespace DCPS
} // namespace OpenDDS
