/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastTransport.h"
#include "MulticastDataLink.h"
#include "MulticastReceiveStrategy.h"
#include "MulticastSendStrategy.h"
#include "MulticastSession.h"
#include "BestEffortSessionFactory.h"
#include "ReliableSessionFactory.h"

#include "ace/Log_Msg.h"
#include "ace/Truncate.h"

#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"

namespace OpenDDS {
namespace DCPS {

MulticastTransport::MulticastTransport(const TransportInst_rch& inst)
  : config_i_(0)
{
  if (!inst.is_nil()) {
    configure(inst.in());
  }
}

MulticastTransport::~MulticastTransport()
{
}

DataLink*
MulticastTransport::find_datalink_i(const RepoId& /*local_id*/,
                                    const RepoId& remote_id,
                                    const TransportBLOB& /*remote_data*/,
                                    CORBA::Long /*priority*/,
                                    bool active)
{
  // To accommodate the one-to-many nature of multicast reservations,
  // a session layer is used to maintain state between unique pairs
  // of DomainParticipants over a single DataLink instance. Given
  // that TransportImpl instances may only be attached to either
  // Subscribers or Publishers within the same DomainParticipant,
  // it may be assumed that the local_id always references the same
  // participant.
  MulticastDataLink_rch link;
  if (active && !this->client_link_.is_nil()) {
    link = this->client_link_;
  }

  if (!active && !this->server_link_.is_nil()) {
    link = this->server_link_;
  }

  if (!link.is_nil()) {

    MulticastPeer remote_peer = RepoIdConverter(remote_id).participantId();

    MulticastSession_rch session = link->find_session(remote_peer);

    if (session.is_nil()) {
      // From the framework's point-of-view, no DataLink was found.
      // This way we will progress to the connect/accept stage for handshaking.
      return 0;
    }

    if (!session->start(active)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("MulticastTransport::find_datalink: ")
                        ACE_TEXT("failed to start session for remote peer: 0x%x!\n"),
                        remote_peer),
                       0);
    }
  }

  return link._retn();
}

MulticastDataLink*
MulticastTransport::make_datalink(const RepoId& local_id,
                                  const RepoId& remote_id,
                                  CORBA::Long priority,
                                  bool active)
{
  RcHandle<MulticastSessionFactory> session_factory;
  if (this->config_i_->reliable_) {
    ACE_NEW_RETURN(session_factory, ReliableSessionFactory, 0);
  } else {
    ACE_NEW_RETURN(session_factory, BestEffortSessionFactory, 0);
  }

  MulticastPeer local_peer = RepoIdConverter(local_id).participantId();
  MulticastPeer remote_peer = RepoIdConverter(remote_id).participantId();

  bool is_loopback = local_peer == remote_peer;

  VDBG_LVL((LM_DEBUG, "(%P|%t) MulticastTransport::make_datalink "
            "remote peer \"%d priority %d is_loopback %d active %d\"\n",
            remote_peer, priority, is_loopback, active),
            2);

  MulticastDataLink_rch link;
  ACE_NEW_RETURN(link,
                 MulticastDataLink(this,
                                   session_factory.in(),
                                   local_peer,
                                   is_loopback,
                                   active),
                 0);

  // Configure link with transport configuration and reactor task:
  link->configure(this->config_i_.in(), reactor_task());

  // Assign send strategy:
  MulticastSendStrategy* send_strategy;
  ACE_NEW_RETURN(send_strategy, MulticastSendStrategy(link.in()), 0);
  link->send_strategy(send_strategy);

  // Assign receive strategy:
  MulticastReceiveStrategy* recv_strategy;
  ACE_NEW_RETURN(recv_strategy, MulticastReceiveStrategy(link.in()), 0);
  link->receive_strategy(recv_strategy);

  // Join multicast group:
  if (!link->join(this->config_i_->group_address_)) {
    ACE_TCHAR str[64];
    this->config_i_->group_address_.addr_to_string(str, sizeof(str));
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport::make_datalink: ")
                      ACE_TEXT("failed to join multicast group: %C!\n"),
                      str),
                     0);
  }
  return link._retn();
}

MulticastSession*
MulticastTransport::start_session(const MulticastDataLink_rch& link,
                                  MulticastPeer remote_peer, bool active)
{
  if (link.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport::start_session: ")
                      ACE_TEXT("link is nil\n")),
                     0);
  }

  MulticastSession_rch session = link->find_or_create_session(remote_peer);
  if (session.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport::start_session: ")
                      ACE_TEXT("failed to create session for remote peer: 0x%x!\n"),
                      remote_peer),
                     0);
  }

  if (!session->start(active)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport::start_session: ")
                      ACE_TEXT("failed to start session for remote peer: 0x%x!\n"),
                      remote_peer),
                     0);
  }

  return session._retn();
}

DataLink*
MulticastTransport::connect_datalink_i(const RepoId& local_id,
                                       const RepoId& remote_id,
                                       const TransportBLOB& /*remote_data*/,
                                       CORBA::Long priority)
{
  MulticastDataLink_rch link = this->client_link_;
  if (link.is_nil()) {
    link = this->make_datalink(local_id, remote_id, priority, true /*active*/);
    this->client_link_ = link;
  }

  MulticastSession_rch session =
    this->start_session(link, RepoIdConverter(remote_id).participantId(),
                        true /*active*/);
  if (session.is_nil()) {
    return 0; // already logged in start_session()
  }

  if (session->wait_for_ack()) {
    return link._retn();
  }

  return 0;
}

DataLink*
MulticastTransport::accept_datalink(ConnectionEvent& ce)
{
  const std::string ttype = "multicast";
  const CORBA::ULong num_blobs = ce.remote_association_.remote_data_.length();
  const RepoId& remote_id = ce.remote_association_.remote_id_;
  MulticastPeer remote_peer = RepoIdConverter(remote_id).participantId();

  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->connections_lock_, 0);

  for (CORBA::ULong idx = 0; idx < num_blobs; ++idx) {
    if (ce.remote_association_.remote_data_[idx].transport_type.in() == ttype) {

      MulticastDataLink_rch link = this->server_link_;
      if (link.is_nil()) {
        link = this->make_datalink(ce.local_id_, remote_id,
                                   ce.priority_, false /*!active*/);
        this->server_link_ = link;
      }

      if (this->connections_.count(remote_peer)) {
        // remote_peer has already completed the handshake
        return link._retn();
      }

      MulticastSession_rch session = this->start_session(link, remote_peer,
                                                         false /*!active*/);
      if (session.is_nil()) {
        return 0; // already logged in start_session()
      }

      this->pending_connections_.insert(std::make_pair(&ce, remote_peer));
      return 0; // can't return link to framework until handshaking is done
    }
  }
  return 0;
}

void
MulticastTransport::stop_accepting(ConnectionEvent& ce)
{
  ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->connections_lock_);
  typedef std::multimap<ConnectionEvent*, MulticastPeer>::iterator iter_t;
  std::pair<iter_t, iter_t> range = this->pending_connections_.equal_range(&ce);
  this->pending_connections_.erase(range.first, range.second);
}

void
MulticastTransport::passive_connection(MulticastPeer peer)
{
  ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->connections_lock_);

  typedef std::multimap<ConnectionEvent*, MulticastPeer>::iterator iter_t;
  for (iter_t iter = this->pending_connections_.begin();
       iter != this->pending_connections_.end(); ++iter) {
    if (iter->second == peer) {
      DataLink_rch link = static_rchandle_cast<DataLink>(this->server_link_);
      iter->first->complete(link);
      std::pair<iter_t, iter_t> range =
        this->pending_connections_.equal_range(iter->first);
      this->pending_connections_.erase(range.first, range.second);
      break;
    }
  }

  this->connections_.insert(peer);
}

bool
MulticastTransport::configure_i(TransportInst* config)
{
  this->config_i_ = dynamic_cast<MulticastInst*>(config);
  if (this->config_i_ == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     false);
  }
  this->config_i_->_add_ref();

  this->create_reactor_task();

  return true;
}

void
MulticastTransport::shutdown_i()
{
  if (!this->client_link_.is_nil()) {
    this->client_link_->transport_shutdown();
  }
  if (!this->server_link_.is_nil()) {
    this->server_link_->transport_shutdown();
  }
  this->config_i_ = 0;
}

bool
MulticastTransport::connection_info_i(TransportLocator& info) const
{
  NetworkAddress network_address(this->config_i_->group_address_);

  ACE_OutputCDR cdr;
  cdr << network_address;

  const CORBA::ULong len = static_cast<CORBA::ULong>(cdr.total_length());
  char* buffer = const_cast<char*>(cdr.buffer()); // safe

  info.transport_type = "multicast";
  info.data = TransportBLOB(len, len, reinterpret_cast<CORBA::Octet*>(buffer));

  return true;
}

void
MulticastTransport::release_datalink_i(DataLink* /*link*/,
                                       bool /*release_pending*/)
{
  this->client_link_ = 0;  // release ownership
  this->server_link_ = 0;  // release ownership
}


} // namespace DCPS
} // namespace OpenDDS
