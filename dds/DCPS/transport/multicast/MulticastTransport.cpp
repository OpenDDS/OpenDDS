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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

MulticastTransport::MulticastTransport(MulticastInst& inst)
  : TransportImpl(inst)
{
  if (! (configure_i(inst) && open())) {
    throw Transport::UnableToCreate();
  }
}

MulticastTransport::~MulticastTransport()
{
}


MulticastInst&
MulticastTransport::config() const
{
  return static_cast<MulticastInst&>(TransportImpl::config());
}

MulticastDataLink_rch
MulticastTransport::make_datalink(const RepoId& local_id,
                                  Priority priority,
                                  bool active)
{

  RcHandle<MulticastSessionFactory> session_factory;

  if (this->config().is_reliable()) {
    session_factory = make_rch<ReliableSessionFactory>();

  } else {
    session_factory = make_rch<BestEffortSessionFactory>();
  }

  MulticastPeer local_peer = (ACE_INT64)RepoIdConverter(local_id).federationId() << 32
                           | RepoIdConverter(local_id).participantId();

  VDBG_LVL((LM_DEBUG, "(%P|%t) MulticastTransport[%C]::make_datalink "
            "peers: local %#08x%08x priority %d active %d\n",
            this->config().name().c_str(), (unsigned int)(local_peer >> 32), (unsigned int)local_peer,
            priority, active), 2);

  ReactorTask_rch rtask(reactor_task());
  MulticastDataLink_rch link(make_rch<MulticastDataLink>(ref(*this),
                                   session_factory,
                                   local_peer,
                                   ref(config()),
                                   rtask.in(),
                                   active));

  // Join multicast group:
  if (!link->join(this->config().group_address_)) {
    ACE_TCHAR str[64];
    this->config().group_address_.addr_to_string(str, sizeof(str)/sizeof(str[0]), 0);
    ACE_ERROR((LM_ERROR,
                    ACE_TEXT("(%P|%t) ERROR: ")
                    ACE_TEXT("MulticastTransport::make_datalink: ")
                    ACE_TEXT("failed to join multicast group: %s!\n"),
                    str));
    return MulticastDataLink_rch();
  }

  return link;
}

MulticastSession_rch
MulticastTransport::start_session(const MulticastDataLink_rch& link,
                                  MulticastPeer remote_peer, bool active)
{
  if (link.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport[%C]::start_session: ")
                      ACE_TEXT("link is nil\n"),
                      this->config().name().c_str()),
                     MulticastSession_rch());
  }

  MulticastSession_rch session(link->find_or_create_session(remote_peer));

  if (session.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport[%C]::start_session: ")
                      ACE_TEXT("failed to create session for remote peer: %#08x%08x!\n"),
                      this->config().name().c_str(),
                      (unsigned int)(remote_peer >> 32),
                      (unsigned int) remote_peer),
                     MulticastSession_rch());
  }

  const bool acked = this->connections_.count(std::make_pair(remote_peer, link->local_peer()));

  if (!session->start(active, acked)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport[%C]::start_session: ")
                      ACE_TEXT("failed to start session for remote peer: %#08x%08x!\n"),
                      this->config().name().c_str(),
                      (unsigned int)(remote_peer >> 32),
                      (unsigned int) remote_peer),
                     MulticastSession_rch());
  }

  return session;
}

static bool
get_remote_reliability(const TransportImpl::RemoteTransport& remote)
{
  NetworkAddress network_address;
  ACE_CDR::Boolean reliable;

  const size_t len = remote.blob_.length();
  const char* buffer = reinterpret_cast<const char*>(remote.blob_.get_buffer());

  ACE_InputCDR cdr(buffer, len);
  cdr >> network_address;
  cdr >> ACE_InputCDR::to_boolean(reliable);

  return reliable;
}

TransportImpl::AcceptConnectResult
MulticastTransport::connect_datalink(const RemoteTransport& remote,
                                     const ConnectionAttribs& attribs,
                                     const TransportClient_rch&)
{
  // Check that the remote reliability matches.
  if (get_remote_reliability(remote) != this->config().is_reliable()) {
    return AcceptConnectResult();
  }

  GuardThreadType guard_links(this->links_lock_);
  const MulticastPeer local_peer = (ACE_INT64)RepoIdConverter(attribs.local_id_).federationId() << 32
                                 | RepoIdConverter(attribs.local_id_).participantId();
  Links::const_iterator link_iter = this->client_links_.find(local_peer);
  MulticastDataLink_rch link;

  if (link_iter == this->client_links_.end()) {
    link = this->make_datalink(attribs.local_id_, attribs.priority_, true /*active*/);
    this->client_links_[local_peer] = link;
  } else {
    link = link_iter->second;
  }

  MulticastPeer remote_peer = (ACE_INT64)RepoIdConverter(remote.repo_id_).federationId() << 32
                            | RepoIdConverter(remote.repo_id_).participantId();

  MulticastSession_rch session(
    this->start_session(link, remote_peer, true /*active*/));

  if (session.is_nil()) {
    Links::iterator to_remove = this->client_links_.find(local_peer);
    if (to_remove != this->client_links_.end()) {
      this->client_links_.erase(to_remove);
    }
    return AcceptConnectResult();
  }
  return AcceptConnectResult(link);
}

TransportImpl::AcceptConnectResult
MulticastTransport::accept_datalink(const RemoteTransport& remote,
                                    const ConnectionAttribs& attribs,
                                    const TransportClient_rch& client)
{
  // Check that the remote reliability matches.
  if (get_remote_reliability(remote) != this->config().is_reliable()) {
    return AcceptConnectResult();
  }

  const MulticastPeer local_peer = (ACE_INT64)RepoIdConverter(attribs.local_id_).federationId() << 32
                                 | RepoIdConverter(attribs.local_id_).participantId();

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

  MulticastPeer remote_peer = (ACE_INT64)RepoIdConverter(remote.repo_id_).federationId() << 32
                            | RepoIdConverter(remote.repo_id_).participantId();
  GuardThreadType guard(this->connections_lock_);

  if (connections_.count(std::make_pair(remote_peer, local_peer))) {
    //can't call start session with connections_lock_ due to reactor
    //call in session->start which could deadlock with passive_connection
    guard.release();

    VDBG((LM_DEBUG, "(%P|%t) MulticastTransport::accept_datalink found\n"));
    MulticastSession_rch session(
      this->start_session(link, remote_peer, false /*!active*/));

    if (session.is_nil()) {
      link.reset();
    }
    return AcceptConnectResult(link);

  } else {

    this->pending_connections_[std::make_pair(remote_peer, local_peer)].
    push_back(std::make_pair(client, remote.repo_id_));
    //can't call start session with connections_lock_ due to reactor
    //call in session->start which could deadlock with passive_connection
    guard.release();
    MulticastSession_rch session(
      this->start_session(link, remote_peer, false /*!active*/));

    return AcceptConnectResult(
      session ? AcceptConnectResult::ACR_SUCCESS : AcceptConnectResult::ACR_FAILED
    );

  }
}

void
MulticastTransport::stop_accepting_or_connecting(const TransportClient_wrch& client,
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
            "from remote peer %#08x%08x to local peer %#08x%08x\n",
            this->config().name().c_str(),
            (unsigned int) (remote_peer >> 32),
            (unsigned int) remote_peer,
            (unsigned int) (local_peer >> 32),
            (unsigned int) local_peer), 2);

  const Peers peers(remote_peer, local_peer);
  const PendConnMap::iterator pend = this->pending_connections_.find(peers);
  //if connection was pending, calls to use_datalink finalized the connection
  //if it was not previously pending, accept_datalink() will finalize connection
  this->connections_.insert(peers);

  Links::const_iterator server_link = this->server_links_.find(local_peer);
  DataLink_rch link;

  if (server_link != this->server_links_.end()) {
    link = static_rchandle_cast<DataLink>(server_link->second);
    MulticastSession_rch session (server_link->second->find_or_create_session(remote_peer));
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
          TransportClient_wrch pend_client = tmp.at(i).first;
          RepoId remote_repo = tmp.at(i).second;
          guard.release();
          TransportClient_rch client = pend_client.lock();
          if (client)
            client->use_datalink(remote_repo, link);
          guard.acquire();
        }
      }
    }
  }
}

bool
MulticastTransport::configure_i(MulticastInst& config)
{
  // Override with DCPSDefaultAddress.
  if (config.local_address_.empty () &&
      !TheServiceParticipant->default_address ().empty ()) {
    config.local_address_ = TheServiceParticipant->default_address ().c_str ();
  }

  if (!config.group_address_.is_multicast()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastTransport[%@]::configure_i: ")
                      ACE_TEXT("invalid configuration: address %C is not ")
                      ACE_TEXT("multicast.\n"),
                      this, this->config().group_address_.get_host_addr()),
                     false);
  }

  this->create_reactor_task(config.async_send_);

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
    if (link->second.in()) {
      link->second->transport_shutdown();
    }
  }
  client_links_.clear();

  for (link = this->server_links_.begin();
       link != this->server_links_.end();
       ++link) {
    if (link->second.in()) {
      link->second->transport_shutdown();
    }
  }
  server_links_.clear();
}

bool
MulticastTransport::connection_info_i(TransportLocator& info, ConnectionInfoFlags flags) const
{
  config().populate_locator(info, flags);
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

OPENDDS_END_VERSIONED_NAMESPACE_DECL
