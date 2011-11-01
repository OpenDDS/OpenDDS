/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "BestEffortSession.h"

namespace OpenDDS {
namespace DCPS {

BestEffortSession::BestEffortSession(MulticastDataLink* link,
                                     MulticastPeer remote_peer)
  : MulticastSession(link, remote_peer)
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
    SequenceRange range(this->expected_, header.sequence_.previous());
    this->reassembly_.data_unavailable(range);
  }

  this->expected_ = header.sequence_;
  ++this->expected_;

  // Assume header is valid; this does not prevent duplicate
  // delivery of datagrams:
  return true;
}

bool
BestEffortSession::check_header(const DataSampleHeader& /*header*/)
{
  // Assume header is valid; this does not prevent duplicate
  // delivery of datagrams:
  return true;
}


bool
BestEffortSession::start(bool active)
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
                      ACE_TEXT("BestEffortSession::start: ")
                      ACE_TEXT("NULL reactor reference!\n")),
                     false);
  }

  this->active_ = active;

  if (active && !this->start_syn(reactor)) {
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
