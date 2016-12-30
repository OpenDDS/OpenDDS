/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastSession.h"

#include "ace/Log_Msg.h"
#include <cmath>
#ifndef __ACE_INLINE__
# include "MulticastSession.inl"
#endif  /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

SynWatchdog::SynWatchdog(ACE_Reactor* reactor,
                         ACE_thread_t owner,
                         MulticastSession* session)
  : DataLinkWatchdog (reactor, owner)
  , session_(session)
  , retries_(0)
{
}

bool
SynWatchdog::reactor_is_shut_down() const
{
  return session_->link()->transport()->is_shut_down();
}

ACE_Time_Value
SynWatchdog::next_interval()
{
  MulticastInst* config = this->session_->link()->config();
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
  MulticastInst* config = this->session_->link()->config();
  return config->syn_timeout_;
}

void
SynWatchdog::on_timeout(const void* /*arg*/)
{
  // There is no recourse if a link is unable to handshake;
  // log an error and return:
  ACE_ERROR((LM_WARNING,
             ACE_TEXT("(%P|%t) WARNING: ")
             ACE_TEXT("SynWatchdog[transport=%C]::on_timeout: ")
             ACE_TEXT("timed out waiting on remote peer: %#08x%08x local: %#08x%08x\n"),
             this->session_->link()->config()->name().c_str(),
             (unsigned int)(this->session_->remote_peer() >> 32),
             (unsigned int) this->session_->remote_peer(),
             (unsigned int)(this->session_->link()->local_peer() >> 32),
             (unsigned int) this->session_->link()->local_peer()));
}


MulticastSession::MulticastSession(ACE_Reactor* reactor,
                                   ACE_thread_t owner,
                                   MulticastDataLink* link,
                                   MulticastPeer remote_peer)
  : link_(link)
  , remote_peer_(remote_peer)
  , reverse_start_lock_(start_lock_)
  , started_(false)
  , active_(true)
  , acked_(false)
  , syn_watchdog_(make_rch<SynWatchdog> (reactor, owner, this))
{
}

MulticastSession::~MulticastSession()
{
  syn_watchdog_->cancel();
  syn_watchdog_->wait();
}

bool
MulticastSession::acked()
{
  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->ack_lock_, false);
  return this->acked_;
}

void
MulticastSession::set_acked() {
  ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->ack_lock_);
  this->acked_ = true;
}

bool
MulticastSession::start_syn()
{
  return this->syn_watchdog_->schedule_now();
}

void
MulticastSession::send_control(char submessage_id, ACE_Message_Block* data)
{
  DataSampleHeader header;
  ACE_Message_Block* control =
    this->link_->create_control(submessage_id, header, data);
  if (control == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("MulticastSession::send_control: ")
               ACE_TEXT("create_control failed!\n")));
    return;
  }

  int error = this->link_->send_control(header, control);
  if (error != SEND_CONTROL_OK) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("MulticastSession::send_control: ")
               ACE_TEXT("send_control failed: %d!\n"),
               error));
    return;
  }
}

bool
MulticastSession::control_received(char submessage_id,
                                   ACE_Message_Block* control)
{
  switch (submessage_id) {
  case MULTICAST_SYN:
    syn_received(control);
    break;

  case MULTICAST_SYNACK:
    synack_received(control);
    break;

  default:
    return false;
  }

  return true;
}

void
MulticastSession::syn_received(ACE_Message_Block* control)
{
  if (this->active_) return; // pub send syn, then doesn't receive them.

  const TransportHeader& header =
    this->link_->receive_strategy()->received_header();

  // Not from the remote peer for this session.
  if (this->remote_peer_ != header.source_) return;

  Serializer serializer(control, header.swap_bytes());

  MulticastPeer local_peer;
  serializer >> local_peer; // sent as remote_peer

  // Ignore sample if not destined for us:
  if (local_peer != this->link_->local_peer()) return;

  VDBG_LVL((LM_DEBUG, "(%P|%t) MulticastSession[%C]::syn_received "
                    "local %#08x%08x remote %#08x%08x\n",
                    this->link()->config()->name().c_str(),
                    (unsigned int)(this->link()->local_peer() >> 32),
                    (unsigned int) this->link()->local_peer(),
                    (unsigned int)(this->remote_peer_ >> 32),
                    (unsigned int) this->remote_peer_), 2);

  {
    ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->ack_lock_);

    if (!this->acked_) {
      this->acked_ = true;
      syn_hook(header.sequence_);
    }
  }

  // MULTICAST_SYN control samples are always positively
  // acknowledged by a matching remote peer:
  send_synack();

  this->link_->transport()->passive_connection(this->link_->local_peer(), this->remote_peer_);

}

void
MulticastSession::send_syn()
{
  size_t len = sizeof(this->remote_peer_);

  ACE_Message_Block* data;
  ACE_NEW(data, ACE_Message_Block(len));

  Serializer serializer(data);

  serializer << this->remote_peer_;

  VDBG_LVL((LM_DEBUG, "(%P|%t) MulticastSession[%C]::send_syn "
                      "local %#08x%08x remote %#08x%08x\n",
                      this->link()->config()->name().c_str(),
                      (unsigned int)(this->link()->local_peer() >> 32),
                      (unsigned int) this->link()->local_peer(),
                      (unsigned int)(this->remote_peer_ >> 32),
                      (unsigned int) this->remote_peer_), 2);

  // Send control sample to remote peer:
  send_control(MULTICAST_SYN, data);
}

void
MulticastSession::synack_received(ACE_Message_Block* control)
{
  if (!this->active_) return; // sub send synack, then doesn't receive them.

  // Already received ack.
  if (this->acked()) return;

  const TransportHeader& header =
    this->link_->receive_strategy()->received_header();

  // Not from the remote peer for this session.
  if (this->remote_peer_ != header.source_) return;

  Serializer serializer(control, header.swap_bytes());

  MulticastPeer local_peer;
  serializer >> local_peer; // sent as remote_peer

  // Ignore sample if not destined for us:
  if (local_peer != this->link_->local_peer()) return;

  VDBG_LVL((LM_DEBUG, "(%P|%t) MulticastSession[%C]::synack_received "
                      "local %#08x%08x remote %#08x%08x\n",
                      this->link()->config()->name().c_str(),
                      (unsigned int)(this->link()->local_peer() >> 32),
                      (unsigned int) this->link()->local_peer(),
                      (unsigned int)(this->remote_peer_ >> 32),
                      (unsigned int) this->remote_peer_), 2);

  {
    ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->ack_lock_);

    if (this->acked_) return; // already acked

    this->syn_watchdog_->cancel();
    this->acked_ = true;
  }
}

void
MulticastSession::send_synack()
{
  size_t len = sizeof(this->remote_peer_);

  ACE_Message_Block* data;
  ACE_NEW(data, ACE_Message_Block(len));

  Serializer serializer(data);

  serializer << this->remote_peer_;

  VDBG_LVL((LM_DEBUG, "(%P|%t) MulticastSession[%C]::send_synack "
                      "local %#08x%08x remote %#08x%08x active %d\n",
                      this->link()->config()->name().c_str(),
                      (unsigned int)(this->link()->local_peer() >> 32),
                      (unsigned int) this->link()->local_peer(),
                      (unsigned int)(this->remote_peer_ >> 32),
                      (unsigned int) this->remote_peer_,
                      this->active_ ? 1 : 0), 2);

  // Send control sample to remote peer:
  send_control(MULTICAST_SYNACK, data);

  // Send naks before sending synack to
  // reduce wait time for resends from remote.
  send_naks();
}

void
MulticastSession::stop()
{
  this->syn_watchdog_->cancel();
}

bool
MulticastSession::reassemble(ReceivedDataSample& data,
                             const TransportHeader& header)
{
  return this->reassembly_.reassemble(header.sequence_,
                                      header.first_fragment_,
                                      data);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
