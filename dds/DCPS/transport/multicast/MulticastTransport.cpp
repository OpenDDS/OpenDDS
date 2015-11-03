/*
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
#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/TransportClient.h"

namespace OpenDDS {
namespace DCPS {

MulticastTransport::MulticastTransport(const TransportInst_rch& inst)
  : config_i_(0)
{
  if (!inst.is_nil()) {
    if (!configure(inst.in())) {
      throw Transport::UnableToCreate();
    }
  }

}

MulticastTransport::~MulticastTransport()
{
}

MulticastDataLink*
MulticastTransport::make_datalink(const RepoId& local_id,
                                  Priority priority,
                                  bool active)
{
  RcHandle<MulticastSessionFactory> session_factory;

  if (this->config_i_->reliable_) {
    ACE_NEW_RETURN(session_factory, ReliableSessionFactory, 0);

  } else {
    ACE_NEW_RETURN(session_factory, BestEffortSessionFactory, 0);
  }

  MulticastPeer local_peer = RepoIdConverter(local_id).participantId();

  VDBG_LVL((LM_DEBUG, "(%P|%t) MulticastTransport[%C]::make_datalink "
            "peers: local 0x%x priority %d active %d\n",
            this->config_i_->name().c_str(), local_peer,
            priority, active), 2);

  MulticastDataLink_rch link;
  ACE_NEW_RETURN(link,
                 MulticastDataLink(this,
                                   session_factory.in(),
                                   local_peer,
                                   active),
                 0);

  // Configure link with transport configuration and reactor task:
  TransportReactorTask_rch rtask = reactor_task();
  //link->configure(this->config_i_.in(), rtask.in());
  link->configure(config_i_.in(), rtask.in());

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
    this->config_i_->group_address_.addr_to_string(str,
                                                   sizeof(str)/sizeof(str[0]));
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport::make_datalink: ")
                      ACE_TEXT("failed to join multicast group: %s!\n"),
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
                      ACE_TEXT("MulticastTransport[%C]::start_session: ")
                      ACE_TEXT("link is nil\n"),
                      this->config_i_->name().c_str()),
                     0);
  }

  MulticastSession_rch session = link->find_or_create_session(remote_peer);

  if (session.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport[%C]::start_session: ")
                      ACE_TEXT("failed to create session for remote peer: 0x%x!\n"),
                      this->config_i_->name().c_str(), remote_peer),
                     0);
  }

  const bool acked = this->connections_.count(std::make_pair(remote_peer, link->local_peer()));

  if (!session->start(active, acked)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport[%C]::start_session: ")
                      ACE_TEXT("failed to start session for remote peer: 0x%x!\n"),
                      this->config_i_->name().c_str(), remote_peer),
                     0);
  }

  return session._retn();
}

static bool
get_remote_reliability(const TransportImpl::RemoteTransport& remote)
{
  NetworkAddress network_address;
  ACE_CDR::Boolean reliable;

  size_t len = remote.blob_.length();
  const char* buffer = reinterpret_cast<const char*>(remote.blob_.get_buffer());

  ACE_InputCDR cdr(buffer, len);
  cdr >> network_address;
  cdr >> ACE_InputCDR::to_boolean(reliable);

  return reliable;
}

TransportImpl::AcceptConnectResult
MulticastTransport::connect_datalink(const RemoteTransport& remote,
                                     const ConnectionAttribs& attribs,
                                     TransportClient*)
{
  // Check that the remote reliability matches.
  if (get_remote_reliability(remote) != this->config_i_->is_reliable()) {
    return AcceptConnectResult();
  }

  GuardThreadType guard_links(this->links_lock_);
  const MulticastPeer local_peer = RepoIdConverter(attribs.local_id_).participantId();
  Links::const_iterator link_iter = this->client_links_.find(local_peer);
  MulticastDataLink_rch link;

  if (link_iter == this->client_links_.end()) {

    link = this->make_datalink(attribs.local_id_, attribs.priority_, true /*active*/);
    this->client_links_[local_peer] = link;
  } else {
    link = link_iter->second;
  }

  MulticastPeer remote_peer = RepoIdConverter(remote.repo_id_).participantId();

  MulticastSession_rch session =
    this->start_session(link, remote_peer, true /*active*/);

  if (session.is_nil()) {
    Links::iterator to_remove = this->client_links_.find(local_peer);
    if (to_remove != this->client_links_.end()) {
      this->client_links_.erase(to_remove);
    }
    return AcceptConnectResult();
  }
  return AcceptConnectResult(link._retn());
}

TransportImpl::AcceptConnectResult
MulticastTransport::accept_datalink(const RemoteTransport& remote,
                                    const ConnectionAttribs& attribs,
                                    TransportClient* client)
{
  // Check that the remote reliability matches.
  if (get_remote_reliability(remote) != this->config_i_->is_reliable()) {
    return AcceptConnectResult();
  }

  const MulticastPeer local_peer = RepoIdConverter(attribs.local_id_).participantId();

  GuardThreadType guard_links(this->links_lock_);

  Links::const_iterator link_iter = this->server_links_.find(local_peer);
  MulticastDataLink_rch link;

  if (link_iter == this->server_links_.end()) {

    link = this->make_datalink(attribs.local_id_, attribs.priority_, false /*passive*/);
    this->server_links_[local_peer] = link;
  } else {
    link = link_iter->second;
  }

  guard_links.release();

  MulticastPeer remote_peer = RepoIdConverter(remote.repo_id_).participantId();
  GuardThreadType guard(this->connections_lock_);

  if (connections_.count(std::make_pair(remote_peer, local_peer))) {
    //can't call start session with connections_lock_ due to reactor
    //call in session->start which could deadlock with passive_connection
    guard.release();

    VDBG((LM_DEBUG, "(%P|%t) MulticastTransport::accept_datalink found\n"));
    MulticastSession_rch session =
      this->start_session(link, remote_peer, false /*!active*/);

    if (session.is_nil()) {
      link = 0;
    }
    return AcceptConnectResult(link._retn());

  } else {

    this->pending_connections_[std::make_pair(remote_peer, local_peer)].
    push_back(std::pair<TransportClient*, RepoId>(client, remote.repo_id_));
    //can't call start session with connections_lock_ due to reactor
    //call in session->start which could deadlock with passive_connection
    guard.release();
    MulticastSession_rch session =
      this->start_session(link, remote_peer, false /*!active*/);

    return AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS);

  }
}

void
MulticastTransport::stop_accepting_or_connecting(TransportClient* client,
                                                 const RepoId& remote_id)
{
  VDBG((LM_DEBUG, "(%P|%t) MulticastTransport::stop_accepting_or_connecting\n"));

  GuardThreadType guard(this->connections_lock_);

  for (PendConnMap::iterator it = this->pending_connections_.begin();
       it != this->pending_connections_.end(); ++it) {
    bool erased_from_it = false;
    for (size_t i = 0; i < it->second.size(); ++i) {
      if (it->second[i].first == client && it->second[i].second == remote_id) {
        erased_from_it = true;
        it->second.erase(it->second.begin() + i);
        break;
      }
    }

    if (erased_from_it && it->second.empty()) {
      this->pending_connections_.erase(it);
      return;
    }
  }
}

void
MulticastTransport::passive_connection(MulticastPeer local_peer, MulticastPeer remote_peer)
{
  GuardThreadType guard(this->connections_lock_);

  VDBG_LVL((LM_DEBUG, "(%P|%t) MulticastTransport[%C]::passive_connection "
            "from remote peer 0x%x to local peer 0x%x\n",
            this->config_i_->name().c_str(), remote_peer, local_peer), 2);

  const Peers peers(remote_peer, local_peer);
  const PendConnMap::iterator pend = this->pending_connections_.find(peers);
  //if connection was pending, calls to use_datalink finalized the connection
  //if it was not previously pending, accept_datalink() will finalize connection
  this->connections_.insert(peers);

  Links::const_iterator server_link = this->server_links_.find(local_peer);
  DataLink_rch link;

  if (server_link != this->server_links_.end()) {
    link = static_rchandle_cast<DataLink>(server_link->second);
    MulticastSession_rch session = server_link->second->find_or_create_session(remote_peer);
    session->set_acked();
  }

  if (pend != pending_connections_.end()) {
    Callbacks tmp(pend->second);
    for (size_t i = 0; i < tmp.size(); ++i) {
      const PendConnMap::iterator pend = pending_connections_.find(peers);
      if (pend != pending_connections_.end()) {
        const Callbacks::iterator tmp_iter = find(pend->second.begin(),
                                                  pend->second.end(),
                                                  tmp.at(i));
        if (tmp_iter != pend->second.end()) {
          TransportClient* pend_client = tmp.at(i).first;
          RepoId remote_repo = tmp.at(i).second;
          guard.release();
          pend_client->use_datalink(remote_repo, link);
          guard.acquire();
        }
      }
    }
  }
}

bool
MulticastTransport::configure_i(TransportInst* config)
{
  this->config_i_ = dynamic_cast<MulticastInst*>(config);

  if (this->config_i_ == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport[%@]::configure_i: ")
                      ACE_TEXT("invalid configuration!\n"), this),
                     false);
  }

  this->config_i_->_add_ref();

  if (!this->config_i_->group_address_.is_multicast()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport[%@]::configure_i: ")
                      ACE_TEXT("invalid configuration: address %C is not ")
                      ACE_TEXT("multicast.\n"),
                      this, this->config_i_->group_address_.get_host_addr()),
                     false);
  }

  this->create_reactor_task(this->config_i_->async_send_);

  return true;
}

void
MulticastTransport::shutdown_i()
{
  GuardThreadType guard_links(this->links_lock_);
  Links::iterator link;

  for (link = this->client_links_.begin();
       link != this->client_links_.end();
       ++link) {
    if (!::CORBA::is_nil(link->second)) {
      link->second->transport_shutdown();
    }
  }

  for (link = this->server_links_.begin();
       link != this->server_links_.end();
       ++link) {
    if (!::CORBA::is_nil(link->second)) {
      link->second->transport_shutdown();
    }
  }

  this->config_i_ = 0;
}

bool
MulticastTransport::connection_info_i(TransportLocator& info) const
{
  this->config_i_->populate_locator(info);
  return true;
}

void
MulticastTransport::release_datalink(DataLink* /*link*/)
{
  // No-op for multicast: keep both the client_link_ and server_link_ around
  // until the transport is shut down.
}

} // namespace DCPS
} // namespace OpenDDS
