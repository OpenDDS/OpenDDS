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

#include "dds/DCPS/async_debug.h"

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

  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::MulticastTransport --> constructed new: %@\n", this));
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
  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::start_session --> enter to create session for remote peer: 0x%x\n", remote_peer));

  if (link.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport[%C]::start_session: ")
                      ACE_TEXT("link is nil\n"),
                      this->config_i_->name().c_str()),
                     0);
  }

  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::start_session --> about to link_->find_or_create_session\n"));

  MulticastSession_rch session = link->find_or_create_session(remote_peer);

  if (session.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport[%C]::start_session: ")
                      ACE_TEXT("failed to create session for remote peer: 0x%x!\n"),
                      this->config_i_->name().c_str(), remote_peer),
                     0);
  }

  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::start_session --> about to have session->start\n"));

  const bool acked = this->connections_.count(std::make_pair(remote_peer, link->local_peer()));

  if (!session->start(active, acked)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport[%C]::start_session: ")
                      ACE_TEXT("failed to start session for remote peer: 0x%x!\n"),
                      this->config_i_->name().c_str(), remote_peer),
                     0);
  }

  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::start_session --> exit, success\n"));

  return session._retn();
}

TransportImpl::AcceptConnectResult
MulticastTransport::connect_datalink(const RemoteTransport& remote,
                                     const ConnectionAttribs& attribs,
                                     TransportClient*)
{
  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> enter\n"));

  GuardThreadType guard_links(this->links_lock_);
  const MulticastPeer local_peer = RepoIdConverter(attribs.local_id_).participantId();
  Links::const_iterator link_iter = this->client_links_.find(local_peer);
  MulticastDataLink_rch link;

  if (link_iter == this->client_links_.end()) {

    //### Debug statements to track where associate is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> link is nil, make_datalink for client_link\n"));

    link = this->make_datalink(attribs.local_id_, attribs.priority_, true /*active*/);
    this->client_links_[local_peer] = link;

  } else {
    link = link_iter->second;
  }

  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> client_link %@ is nil? %s\n", link.in(), link.is_nil() ? "YES":"NO"));

  MulticastPeer remote_peer = RepoIdConverter(remote.repo_id_).participantId();

  //### Debug statements to track where associate is failing
  GuidConverter remote_repo(remote.repo_id_);

  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> trying to connect to remote: %C\n", std::string(remote_repo).c_str()));

  MulticastSession_rch session =
    this->start_session(link, remote_peer, true /*active*/);

  if (session.is_nil()) {
    //### Debug statements to track where associate is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> exit, FAILURE, session is nil\n"));

    return AcceptConnectResult();
  }

  return AcceptConnectResult(link._retn());
}

TransportImpl::AcceptConnectResult
MulticastTransport::accept_datalink(const RemoteTransport& remote,
                                    const ConnectionAttribs& attribs,
                                    TransportClient* client)
{
  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> enter\n"));

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
    //###can't call start session with connections_lock_ due to reactor call in session->start which could deadlock with passive_connection
    guard.release();

    VDBG((LM_DEBUG, "(%P|%t) MulticastTransport::accept_datalink found\n"));
    MulticastSession_rch session =
      this->start_session(link, remote_peer, false /*!active*/);

    if (session.is_nil()) {
      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> found connection, but session is NIL\n"));

      link = 0;
    }

    //### Debug statements to track where associate is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> exit SUCCESS with link\n"));

    //### Debug statements to track where associate is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> link is nil? %s\n", link.is_nil() ? "YES":"NO"));

    return AcceptConnectResult(link._retn());

  } else {

    this->pending_connections_[std::make_pair(remote_peer, local_peer)].
    push_back(std::pair<TransportClient*, RepoId>(client, remote.repo_id_));
    //###can't call start session with connections_lock_ due to reactor call in session->start which could deadlock with passive_connection
    guard.release();
    MulticastSession_rch session =
      this->start_session(link, remote_peer, false /*!active*/);

    //### Debug statements to track where associate is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> exit SUCCESS with no link, added callback\n"));

    return AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS);

  }
}

void
MulticastTransport::stop_accepting_or_connecting(TransportClient* client,
                                                 const RepoId& remote_id)
{
  VDBG((LM_DEBUG, "(%P|%t) MulticastTransport::stop_accepting_or_connecting\n"));

  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::stop_accepting_or_connection --> enter\n"));

  GuardThreadType guard(this->connections_lock_);

  for (PendConnMap::iterator it = this->pending_connections_.begin();
       it != this->pending_connections_.end(); ++it) {
    for (size_t i = 0; i < it->second.size(); ++i) {
      if (it->second[i].first == client && it->second[i].second == remote_id) {
        it->second.erase(it->second.begin() + i);
        break;
      }
    }

    if (it->second.empty()) {
      this->pending_connections_.erase(it);
      return;
    }
  }

  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::stop_accepting_or_connecting --> exit\n"));
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

  if (pend != pending_connections_.end()) {

    //How many callbacks are there??
    //### Debug statements to track where associate is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::passive_connection --> num callbacks = %d\n", pend->second.size()));

    Links::const_iterator server_link = this->server_links_.find(local_peer);
    DataLink_rch link;

    if (server_link != this->server_links_.end()) {
      link = static_rchandle_cast<DataLink>(server_link->second);
    }

    VDBG((LM_DEBUG, "(%P|%t) MulticastTransport::passive_connection completing\n"));
    PendConnMap::iterator updated_pend = pend;

    do {
      TransportClient* pend_client = updated_pend->second.front().first;
      RepoId remote_repo = updated_pend->second.front().second;

      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::passive_connection --> about to call use_datalink in loop\n"));

      guard.release();
      pend_client->use_datalink(remote_repo, link);

      guard.acquire();

      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::passive_connection --> finished use_datalink in loop\n"));
    } while ((updated_pend = pending_connections_.find(peers)) != pending_connections_.end());
  }

  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::passive_connection --> inserting peer into connections_\n"));

  //if connection was pending, calls to use_datalink finalized the connection
  //if it was not previously pending, accept_datalink() will finalize connection
  this->connections_.insert(peers);
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
    link->second->transport_shutdown();
  }

  for (link = this->server_links_.begin();
       link != this->server_links_.end();
       ++link) {
    link->second->transport_shutdown();
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
MulticastTransport::release_datalink(DataLink* /*link*/)
{
  // No-op for multicast: keep both the client_link_ and server_link_ around
  // until the transport is shut down.
}

} // namespace DCPS
} // namespace OpenDDS
