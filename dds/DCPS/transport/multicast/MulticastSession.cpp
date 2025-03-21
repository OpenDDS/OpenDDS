/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastSession.h"
#include "MulticastReceiveStrategy.h"

#include <dds/DCPS/GuidConverter.h>

#include <ace/Log_Msg.h>

#include <cmath>


#ifndef __ACE_INLINE__
# include "MulticastSession.inl"
#endif  /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

namespace {
  const Encoding::Kind encoding_kind = Encoding::KIND_UNALIGNED_CDR;
}

MulticastSession::MulticastSession(RcHandle<ReactorTask> reactor_task,
                                   MulticastDataLink* link,
                                   MulticastPeer remote_peer)
  : link_(link)
  , remote_peer_(remote_peer)
  , reverse_start_lock_(start_lock_)
  , started_(false)
  , active_(true)
  , reassembly_(link->config()->fragment_reassembly_timeout())
  , acked_(false)
  , syn_watchdog_(make_rch<Sporadic>(TheServiceParticipant->time_source(),
                                     reactor_task,
                                     rchandle_from(this),
                                     &MulticastSession::send_all_syn))
  , initial_syn_delay_(link->config()->syn_interval())
  , config_name(link->config()->name())
{}

MulticastSession::~MulticastSession()
{
  syn_watchdog_->cancel();
}

bool
MulticastSession::acked()
{
  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->ack_lock_, false);
  return this->acked_;
}

void
MulticastSession::set_acked()
{
  ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->ack_lock_);
  this->acked_ = true;
}

void
MulticastSession::start_syn()
{
  syn_watchdog_->cancel();
  syn_delay_ = initial_syn_delay_;
  syn_watchdog_->schedule(TimeDuration(0));
}

void
MulticastSession::send_control(char submessage_id, Message_Block_Ptr data)
{
  DataSampleHeader header;
  Message_Block_Ptr control(this->link_->create_control(submessage_id, header, OPENDDS_MOVE_NS::move(data)));
  if (!control) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("MulticastSession::send_control: ")
               ACE_TEXT("create_control failed!\n")));
    return;
  }

  int error = this->link_->send_control(header, OPENDDS_MOVE_NS::move(control));
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
                                   const Message_Block_Ptr& control)
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
MulticastSession::syn_received(const Message_Block_Ptr& control)
{
  if (this->active_) return; // pub send syn, then doesn't receive them.

  const TransportHeader& header =
    this->link_->receive_strategy()->received_header();

  // Not from the remote peer for this session.
  if (this->remote_peer_ != header.source_) return;

  Serializer serializer(control.get(), encoding_kind, header.swap_bytes());

  MulticastPeer local_peer;
  GUID_t remote_writer;
  GUID_t local_reader;
  serializer >> local_peer; // sent as remote_peer
  serializer.read_octet_array(reinterpret_cast<ACE_CDR::Octet*>(&remote_writer), sizeof(remote_writer));
  serializer.read_octet_array(reinterpret_cast<ACE_CDR::Octet*>(&local_reader), sizeof(local_reader));

  // Ignore sample if not destined for us:
  if (local_peer != this->link_->local_peer()) return;

  bool call_passive_connection = false;
  bool call_send_synack = true;
  {
    ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->ack_lock_);
    PendingRemoteMap::const_iterator pos1 = pending_remote_map_.find(local_reader);
    if (pos1 == pending_remote_map_.end()) {
      call_send_synack = false;
    } else {
      RepoIdSet::const_iterator pos2 = pos1->second.find(remote_writer);
      if (pos2 == pos1->second.end()) {
        call_send_synack = false;
      }
    }

    VDBG_LVL((LM_DEBUG,
              "(%P|%t) MulticastSession[%C]::syn_received "
              "local %#08x%08x %C remote %#08x%08x %C\n",
              config_name.c_str(),
              (unsigned int)(this->link()->local_peer() >> 32),
              (unsigned int) this->link()->local_peer(),
              LogGuid(local_reader).c_str(),
              (unsigned int)(this->remote_peer_ >> 32),
              (unsigned int) this->remote_peer_,
              LogGuid(remote_writer).c_str()),
             2);

    if (!this->acked_) {
      this->acked_ = true;
      syn_hook(header.sequence_);
      call_passive_connection = true;
    }
  }

  if (call_passive_connection) {
    MulticastTransport_rch transport = link_->transport();
    if (transport) {
      transport->passive_connection(link_->local_peer(), remote_peer_);
    }
  }

  // MULTICAST_SYN control samples are always positively
  // acknowledged by a matching remote peer:
  if (call_send_synack) {
    send_synack(local_reader, remote_writer);
  }
}

void
MulticastSession::send_all_syn(const MonotonicTimePoint& /*now*/)
{
  ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->ack_lock_);
  for (PendingRemoteMap::const_iterator pos1 = pending_remote_map_.begin(), limit1 = pending_remote_map_.end();
       pos1 != limit1; ++pos1) {
    const GUID_t& local_writer = pos1->first;
    for (RepoIdSet::const_iterator pos2 = pos1->second.begin(), limit2 = pos1->second.end(); pos2 != limit2; ++pos2) {
      const GUID_t& remote_reader = *pos2;
      send_syn(local_writer, remote_reader);
    }
  }

  // Exponential back-off.
  syn_delay_ *= 2;
  syn_watchdog_->schedule(syn_delay_);
}

void
MulticastSession::send_syn(const GUID_t& local_writer,
                           const GUID_t& remote_reader)
{
  size_t len = sizeof(this->remote_peer_) + 2 * sizeof(GUID_t);

  Message_Block_Ptr data( new ACE_Message_Block(len));

  Serializer serializer(data.get(), encoding_kind);

  serializer << this->remote_peer_;
  serializer.write_octet_array(reinterpret_cast<const ACE_CDR::Octet*>(&local_writer), sizeof(local_writer));
  serializer.write_octet_array(reinterpret_cast<const ACE_CDR::Octet*>(&remote_reader), sizeof(remote_reader));

  VDBG_LVL((LM_DEBUG,
            "(%P|%t) MulticastSession[%C]::send_syn "
            "local %#08x%08x %C remote %#08x%08x %C\n",
            config_name.c_str(),
            (unsigned int)(this->link()->local_peer() >> 32),
            (unsigned int) this->link()->local_peer(),
            LogGuid(local_writer).c_str(),
            (unsigned int)(this->remote_peer_ >> 32),
            (unsigned int) this->remote_peer_,
            LogGuid(remote_reader).c_str()),
           2);

  // Send control sample to remote peer:
  send_control(MULTICAST_SYN, OPENDDS_MOVE_NS::move(data));
}

void
MulticastSession::synack_received(const Message_Block_Ptr& control)
{
  if (!this->active_) return; // sub send synack, then doesn't receive them.

  // Already received ack.
  //if (this->acked()) return;

  const TransportHeader& header =
    this->link_->receive_strategy()->received_header();

  // Not from the remote peer for this session.
  if (this->remote_peer_ != header.source_) return;

  Serializer serializer(control.get(), encoding_kind, header.swap_bytes());

  MulticastPeer local_peer;
  GUID_t remote_reader;
  GUID_t local_writer;
  serializer >> local_peer; // sent as remote_peer
  serializer.read_octet_array(reinterpret_cast<ACE_CDR::Octet*>(&remote_reader), sizeof(remote_reader));
  serializer.read_octet_array(reinterpret_cast<ACE_CDR::Octet*>(&local_writer), sizeof(local_writer));

  // Ignore sample if not destined for us:
  if (local_peer != this->link_->local_peer()) return;

  VDBG_LVL((LM_DEBUG,
            "(%P|%t) MulticastSession[%C]::synack_received "
            "local %#08x%08x %C remote %#08x%08x %C\n",
            config_name.c_str(),
            (unsigned int)(this->link()->local_peer() >> 32),
            (unsigned int) this->link()->local_peer(),
            LogGuid(local_writer).c_str(),
            (unsigned int)(this->remote_peer_ >> 32),
            (unsigned int) this->remote_peer_,
            LogGuid(remote_reader).c_str()),
           2);

  {
    ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->ack_lock_);
    this->acked_ = true;
    remove_remote_i(local_writer, remote_reader);
  }

  this->link_->invoke_on_start_callbacks(local_writer, remote_reader, true);
}

void
MulticastSession::send_synack(const GUID_t& local_reader,
                              const GUID_t& remote_writer)
{
  size_t len = sizeof(this->remote_peer_) + 2 * sizeof(GUID_t);

  Message_Block_Ptr data(new ACE_Message_Block(len));

  Serializer serializer(data.get(), encoding_kind);

  serializer << this->remote_peer_;
  serializer.write_octet_array(reinterpret_cast<const ACE_CDR::Octet*>(&local_reader), sizeof(local_reader));
  serializer.write_octet_array(reinterpret_cast<const ACE_CDR::Octet*>(&remote_writer), sizeof(remote_writer));

  VDBG_LVL((LM_DEBUG, "(%P|%t) MulticastSession[%C]::send_synack "
                      "local %#08x%08x %C remote %#08x%08x %C active %d\n",
                      config_name.c_str(),
                      (unsigned int)(this->link()->local_peer() >> 32),
                      (unsigned int) this->link()->local_peer(),
                      LogGuid(local_reader).c_str(),
                      (unsigned int)(this->remote_peer_ >> 32),
                      (unsigned int) this->remote_peer_,
                      LogGuid(remote_writer).c_str(),
                      this->active_ ? 1 : 0), 2);

  // Send control sample to remote peer:
  send_control(MULTICAST_SYNACK, OPENDDS_MOVE_NS::move(data));

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

void
MulticastSession::add_remote(const GUID_t& local)
{
  const GuidConverter conv(local);
  if (conv.isWriter()) {
    // Active peers schedule a watchdog timer to initiate a 2-way
    // handshake to verify that passive endpoints can send/receive
    // data reliably. This process must be executed using the
    // transport reactor thread to prevent blocking.
    // Only publisher send syn so just schedule for pub role.
    this->start_syn();
  }
}

void
MulticastSession::add_remote(const GUID_t& local,
                             const GUID_t& remote)
{
  const GuidConverter conv(local);

  {
    ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->ack_lock_);
    pending_remote_map_[local].insert(remote);
  }

  if (conv.isWriter()) {
    // Active peers schedule a watchdog timer to initiate a 2-way
    // handshake to verify that passive endpoints can send/receive
    // data reliably. This process must be executed using the
    // transport reactor thread to prevent blocking.
    // Only publisher send syn so just schedule for pub role.
    this->start_syn();
  }
}

void
MulticastSession::remove_remote(const GUID_t& local,
                                const GUID_t& remote)
{
  ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->ack_lock_);
  remove_remote_i(local, remote);
}

void
MulticastSession::remove_remote_i(const GUID_t& local,
                                  const GUID_t& remote)
{
  const GuidConverter conv(local);

  const bool empty_before = pending_remote_map_.empty();
  pending_remote_map_[local].erase(remote);
  if (pending_remote_map_[local].empty()) {
    pending_remote_map_.erase(local);
  }
  const bool empty = pending_remote_map_.empty() && !empty_before;

  if (conv.isWriter() && empty && this->syn_watchdog_) {
    this->syn_watchdog_->cancel();
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
