/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "BestEffortSession.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

BestEffortSession::BestEffortSession(ACE_Reactor* reactor,
                                     ACE_thread_t owner,
                                     MulticastDataLink* link,
                                     MulticastPeer remote_peer)
  : MulticastSession(reactor, owner, link, remote_peer)
  , expected_(SequenceNumber::SEQUENCENUMBER_UNKNOWN())
{
}

bool
BestEffortSession::check_header(const TransportHeader& header)
{
  if (header.sequence_ != this->expected_ &&
      expected_ != SequenceNumber::SEQUENCENUMBER_UNKNOWN()) {
    VDBG_LVL((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: BestEffortSession::check_header ")
               ACE_TEXT("expected %q received %q\n"),
               this->expected_.getValue(), header.sequence_.getValue()), 2);
    if (header.sequence_ > this->expected_) {
      SequenceRange range(this->expected_, header.sequence_.previous());
      this->reassembly_.data_unavailable(range);
    }
  }

  this->expected_ = header.sequence_;
  ++this->expected_;

  // Assume header is valid; this does not prevent duplicate
  // delivery of datagrams:
  return true;
}

void
BestEffortSession::record_header_received(const TransportHeader& header)
{
  if (this->remote_peer_ != header.source_) return;

  check_header(header);
}

bool
BestEffortSession::ready_to_deliver(const TransportHeader& header,
                                    const ReceivedDataSample& /*data*/)
{
  if (expected_ != SequenceNumber::SEQUENCENUMBER_UNKNOWN()
      && header.sequence_ == expected_.previous()) {
    return true;
  }
  return false;
}

bool
BestEffortSession::start(bool active, bool /*acked*/)
{
  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX,
                   guard,
                   this->start_lock_,
                   false);

  if (this->started_) return true;  // already started

  this->active_ = active;

  if (active && !this->start_syn()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("BestEffortSession::start: ")
                      ACE_TEXT("failed to schedule SYN watchdog!\n")),
                     false);
  }

  return this->started_ = true;
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
