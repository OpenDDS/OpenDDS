/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastDataLink.h"
#include "MulticastSession.h"
#include "MulticastSessionFactory.h"
#include "MulticastTransport.h"

#include "ace/Global_Macros.h"
#include "ace/Log_Msg.h"

#include "dds/DCPS/RepoIdBuilder.h"

#ifndef __ACE_INLINE__
# include "MulticastDataLink.inl"
#endif  /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

MulticastDataLink::MulticastDataLink(MulticastTransport* transport,
                                     MulticastSessionFactory* session_factory,
                                     MulticastPeer local_peer)
  : DataLink(transport, 0), // priority
    transport_(transport),
    session_factory_(session_factory),
    local_peer_(local_peer),
    config_(0),
    reactor_task_(0)
{
}

MulticastDataLink::~MulticastDataLink()
{
  if (!this->send_buffer_.is_nil()) {
    this->send_strategy_->send_buffer(0);
  }
}

void
MulticastDataLink::configure(MulticastConfiguration* config,
                             TransportReactorTask* reactor_task)
{
  this->config_ = config;
  this->reactor_task_ = reactor_task;
}

void
MulticastDataLink::send_strategy(MulticastSendStrategy* send_strategy)
{
  // A send buffer may be bound to the send strategy to ensure a
  // configured number of most-recent datagrams are retained:
  if (this->session_factory_->requires_send_buffer()) {
    ACE_NEW_NORETURN(this->send_buffer_,
                     TransportSendBuffer(this->config_->nak_depth_,
                                         this->config_->max_samples_per_packet_));
    if (this->send_buffer_.is_nil()) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("MulticastDataLink::send_strategy: ")
                 ACE_TEXT("failed to create TransportSendBuffer!\n")));
      return;
    }
    send_strategy->send_buffer(this->send_buffer_.in());
  }
  this->send_strategy_ = send_strategy;
}

void
MulticastDataLink::receive_strategy(MulticastReceiveStrategy* recv_strategy)
{
  this->recv_strategy_ = recv_strategy;
}

bool
MulticastDataLink::join(const ACE_INET_Addr& group_address)
{
  if (this->socket_.join(group_address) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastDataLink::join: ")
                      ACE_TEXT("ACE_SOCK_Dgram_Mcast::join failed: %p\n")),
                     false);
  }

  if (start(this->send_strategy_.in(), this->recv_strategy_.in()) != 0) {
    this->socket_.close();
    ACE_ERROR_RETURN((LM_ERROR,
		      ACE_TEXT("(%P|%t) ERROR: ")
		      ACE_TEXT("MulticastDataLink::join: ")
		      ACE_TEXT("DataLink::start failed!\n")),
                     false);
  }

  return true;
}

bool
MulticastDataLink::obtain_session(MulticastPeer remote_peer, bool active)
{
  ACE_GUARD_RETURN(ACE_SYNCH_RECURSIVE_MUTEX,
                   guard,
                   this->session_lock_,
                   false);

  MulticastSessionMap::iterator it(this->sessions_.find(remote_peer));
  if (it != this->sessions_.end()) return true; // already exists

  MulticastSession_rch session =
    this->session_factory_->create(this, remote_peer);
  if (session.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastDataLink::obtain_session: ")
                      ACE_TEXT("failed to create session for remote peer: 0x%x!\n"),
                      remote_peer),
                     false);
  }

  if (!session->start(active)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastDataLink::obtain_session: ")
                      ACE_TEXT("failed to start session for remote peer: 0x%x!\n"),
                      remote_peer),
                     false);
  }

  std::pair<MulticastSessionMap::iterator, bool> pair = this->sessions_.insert(
    MulticastSessionMap::value_type(remote_peer, session));
  if (pair.first == this->sessions_.end()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastStrategy::obtain_session: ")
                      ACE_TEXT("failed to insert session for remote peer: 0x%x!\n"),
                      remote_peer),
                     false);
  }

  return true;
}

bool
MulticastDataLink::acked(MulticastPeer remote_peer)
{
  ACE_GUARD_RETURN(ACE_SYNCH_RECURSIVE_MUTEX,
                   guard,
                   this->session_lock_,
                   false);

  MulticastSessionMap::iterator it(this->sessions_.find(remote_peer));
  if (it == this->sessions_.end()) return false;  // unknown peer

  return it->second->acked();
}

bool
MulticastDataLink::header_received(const TransportHeader& header)
{
  ACE_GUARD_RETURN(ACE_SYNCH_RECURSIVE_MUTEX,
                   guard,
                   this->session_lock_,
                   false);

  MulticastSessionMap::iterator it(this->sessions_.find(header.source_));
  if (it == this->sessions_.end()) return true;  // unknown peer

  return it->second->header_received(header);
}

void
MulticastDataLink::sample_received(ReceivedDataSample& sample)
{
  switch (sample.header_.message_id_) {
  case TRANSPORT_CONTROL: {
    // Transport control samples are delivered to all sessions
    // regardless of association status:
    ACE_GUARD(ACE_SYNCH_RECURSIVE_MUTEX,
              guard,
              this->session_lock_);

    for (MulticastSessionMap::iterator it(this->sessions_.begin());
         it != this->sessions_.end(); ++it) {
      it->second->control_received(sample.header_.submessage_id_,
                                   sample.sample_);
    }

  } break;

  case SAMPLE_ACK:
    ack_received(sample);
    break;

  default:
    data_received(sample);
  }
}

void
MulticastDataLink::reliability_lost(const InterfaceListType& interfaces)
{
  // Sessions which have lost reliability are marked defunct;
  // copy affected peer identifiers and destroy session state:
  std::list<MulticastPeer> remote_peers;
  {
    ACE_GUARD(ACE_SYNCH_RECURSIVE_MUTEX,
              guard,
              this->session_lock_);

    MulticastSessionMap::iterator it(this->sessions_.begin());
    while (it != this->sessions_.end()) {
      MulticastSessionMap::iterator prev(it++);

      if (prev->second->defunct()) {
        MulticastSession_rch session = prev->second;

        session->stop();
        remote_peers.push_back(session->remote_peer());

        this->sessions_.erase(prev);
      }
    }
  }

  if (remote_peers.empty()) return; // nothing to disassociate

  for (InterfaceListType::const_iterator it(interfaces.begin());
       it != interfaces.end(); ++it) {

    TransportInterface* intf = *it;

    for (std::list<MulticastPeer>::iterator peer(remote_peers.begin());
         peer != remote_peers.end(); ++peer) {
      // Reconstruct the remote participant RepoId by substituting
      // the local participantId with the remote peer value:
      RepoId remote_id(intf->get_participant_id());

      RepoIdBuilder builder(remote_id);
      builder.participantId(*peer);

      intf->disassociate_participant(remote_id);
    }
  }
}

void
MulticastDataLink::stop_i()
{
  ACE_GUARD(ACE_SYNCH_RECURSIVE_MUTEX,
            guard,
            this->session_lock_);

  for (MulticastSessionMap::iterator it(this->sessions_.begin());
       it != this->sessions_.end(); ++it) {
    it->second->stop();
  }
  this->sessions_.clear();

  this->socket_.close();
}

} // namespace DCPS
} // namespace OpenDDS
