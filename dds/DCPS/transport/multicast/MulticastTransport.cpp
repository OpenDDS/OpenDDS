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

////***DataLink*
////MulticastTransport::find_datalink_i(const RepoId& /*local_id*/,
////                                    const RepoId& remote_id,
////                                    const TransportBLOB& /*remote_data*/,
////                                    bool /*remote_reliable*/,
////                                    bool /*remote_durable*/,
////                                    const ConnectionAttribs& /*attribs*/,
////                                    bool active)
////***
///***  For reference on RemoteTransport from TransportImpl
//    struct RemoteTransport {
//    RepoId repo_id_;
//    TransportBLOB blob_;
//    Priority publication_transport_priority_;
//    bool reliable_, durable_;
//  };
//***/
//TransportImpl::AcceptConnectResult
//MulticastTransport::connect_datalink(const RemoteTransport& remote,
//                                     const ConnectionAttribs& attribs,
//                                     TransportClient* client)
//{
//  // To accommodate the one-to-many nature of multicast reservations,
//  // a session layer is used to maintain state between unique pairs
//  // of DomainParticipants over a single DataLink instance. Given
//  // that TransportImpl instances may only be attached to either
//  // Subscribers or Publishers within the same DomainParticipant,
//  // it may be assumed that the local_id always references the same
//  // participant.
//  MulticastDataLink_rch link;
///***  if (active && !this->client_link_.is_nil()) {
//    link = this->client_link_;
//  }
//
//  if (!active && !this->server_link_.is_nil()) {
//    link = this->server_link_;
//  }
//***/
//  const bool active = true;
//
//  if(!this->client_link_.is_nil()) {
//    link = this->client_link_;
//  }
//
//  if (!link.is_nil()) {
//
//    //***MulticastPeer remote_peer = RepoIdConverter(remote_id).participantId();
//	MulticastPeer remote_peer = RepoIdConverter(remote.repo_id_).participantId();
//
//	MulticastSession_rch session = link->find_or_create_session(remote_peer);
///***    MulticastSession_rch session = link->find_session(remote_peer);
//
//    if (session.is_nil()) {
//      // From the framework's point-of-view, no DataLink was found.
//      // This way we will progress to the connect/accept stage for handshaking.
//      return 0;
//    }
//***/
//    if (!session->start(active, this->connections_.count(remote_peer))) {
//      ACE_ERROR_RETURN((LM_ERROR,
//                        ACE_TEXT("(%P|%t) ERROR: ")
//                        /***ACE_TEXT("MulticastTransport[%C]::find_datalink_i: ")***/
//                        ACE_TEXT("MulticastTransport[%C]::connect_datalink: ")
//                        ACE_TEXT("failed to start session for remote peer: 0x%x!\n"),
//                        this->config_i_->name().c_str(), remote_peer),
//                       0);
//    }
//
//    /***VDBG_LVL((LM_DEBUG, "(%P|%t) MulticastTransport[%C]::find_datalink_i "
//              "started session for remote peer: 0x%x\n",
//              this->config_i_->name().c_str(), remote_peer), 2);***/
//    VDBG_LVL((LM_DEBUG, "(%P|%t) MulticastTransport[%C]::connect_datalink "
//              "started session for remote peer: 0x%x\n",
//              this->config_i_->name().c_str(), remote_peer), 2);
//    return AcceptConnectResult(link._retn());
//  }
//
//  //*** If we arrived here the link was nil so need to make datalink
//
//
//  return AcceptConnectResult();
//  //***return link._retn();
//}

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
      TransportClient* )
{
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> enter\n"));


   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> trying to LOCK links_lock_\n"));
   GuardThreadType guard_links(this->links_lock_);
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> LOCKED links_lock_\n"));
  const MulticastPeer local_peer = RepoIdConverter(attribs.local_id_).participantId();
  Links::const_iterator link_iter = this->client_links_.find(local_peer);
  MulticastDataLink_rch link;
  if (link_iter == this->client_links_.end()) {

    //### Debug statements to track where associate is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> link is nil, make_datalink for client_link\n"));
    link = this->make_datalink(attribs.local_id_, attribs.priority_, true /*active*/);
    this->client_links_[local_peer] = link;
//### not sure why the DataWriterImpl (active) side was creating a server link
//      if (this->server_link_.is_nil()) {
//         //### Debug statements to track where associate is failing
//         if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> server_link_ is nil, make_datalink\n"));
//         // Create the "server" link now, so that it can receive MULTICAST_SYN
//         // from any peers that have add_association() first.
//         this->server_link_ = make_datalink(attribs.local_id_, attribs.priority_,
//               false /*active*/);
//      }
   }
  else {
    link = link_iter->second;
  }

   //### Debug statements to track where associate is failing
   //if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> RELEASING links_lock_\n"));
   //guard_links.release();

   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> client_link %@ is nil? %s\n", link.in(), link.is_nil() ? "YES":"NO"));

   MulticastPeer remote_peer = RepoIdConverter(remote.repo_id_).participantId();

   //### Debug statements to track where associate is failing
   GuidConverter remote_repo(remote.repo_id_);
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> trying to connect to remote: %C\n", std::string(remote_repo).c_str() ));


   MulticastSession_rch session =
         this->start_session(link, remote_peer, true /*active*/);
   if (session.is_nil()) {
      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> exit, FAILURE, session is nil\n"));
      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> RELEASING links_lock_\n"));
      return AcceptConnectResult();
   }

   if (remote_peer == RepoIdConverter(attribs.local_id_).participantId()) {
      VDBG_LVL((LM_DEBUG, "(%P|%t) MulticastTransport[%C]::connect_datalink_i "
            "loopback on peer: 0x%x, skipping wait_for_ack\n",
            this->config_i_->name().c_str(), remote_peer), 2);
      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> exit SUCCESS w/link, loopback\n"));
      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> link is nil? %s\n", link.is_nil() ? "YES":"NO"));
      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> RELEASING links_lock_\n"));
      return AcceptConnectResult(link._retn());
   }
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> RELEASING links_lock_\n"));
   return AcceptConnectResult(link._retn());
   /*
   if (session->acked()) {
      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> exit SUCCESS w/ link, session acked\n"));
      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> link is nil? %s\n", link.is_nil() ? "YES":"NO"));
      return AcceptConnectResult(link._retn());
   }
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::connect_datalink --> exit SUCCESS, nil link (catch_all)\n"));
   return AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS);\
    */
}

TransportImpl::AcceptConnectResult
MulticastTransport::accept_datalink(const RemoteTransport& remote,
      const ConnectionAttribs& attribs,
      TransportClient* client)
{
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> enter\n"));
  const MulticastPeer local_peer = RepoIdConverter(attribs.local_id_).participantId();

   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> trying to LOCK links_lock_\n"));
   GuardThreadType guard_links(this->links_lock_);
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> LOCKED links_lock_\n"));

  Links::const_iterator link_iter = this->server_links_.find(local_peer);
  MulticastDataLink_rch link;

  if (link_iter == this->server_links_.end()) {

    link = this->make_datalink(attribs.local_id_, attribs.priority_, false /*passive*/);
    this->server_links_[local_peer] = link;
  }
  else {
    link = link_iter->second;
  }
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> RELEASING links_lock_\n"));

   guard_links.release();

   MulticastPeer remote_peer = RepoIdConverter(remote.repo_id_).participantId();
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> trying to LOCK connections_lock_\n"));
   GuardThreadType guard(this->connections_lock_);
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> LOCKED connections_lock_\n"));
  if (connections_.count(std::make_pair(remote_peer, local_peer))) {
    //### Debug statements to track where associate is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> connection found, RELEASE connections_lock_, start session\n"));
    //###can't call start session with connections_lock_ due to reactor call in session->start which could deadlock with passive_connection
    guard.release();

    VDBG((LM_DEBUG, "(%P|%t) MulticastTransport::accept_datalink found\n"));
    MulticastSession_rch session =
    this->start_session(link, remote_peer, false /*!active*/);

      if (session.is_nil()){
         //### Debug statements to track where associate is failing
         if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> found connection, but session is NIL\n"));
         link = 0;
      }
      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> RELEASE connections_lock_\n"));
      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> exit SUCCESS with link\n"));
      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> link is nil? %s\n", link.is_nil() ? "YES":"NO"));
      return AcceptConnectResult(link._retn());
  } else {
      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> no connection found, add callback, RELEASE connections_lock_ and try to start_session\n"));

    this->pending_connections_[std::make_pair(remote_peer, local_peer)].
      push_back(std::pair<TransportClient*, RepoId>(client, remote.repo_id_));
      //###can't call start session with connections_lock_ due to reactor call in session->start which could deadlock with passive_connection
      guard.release();
    MulticastSession_rch session =
      this->start_session(link, remote_peer, false /*!active*/);
      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::accept_datalink --> RELEASE connections_lock_\n"));
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
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::stop_accepting_or_connecting --> trying to LOCK connections_lock_\n"));
   GuardThreadType guard(this->connections_lock_);
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::stop_accepting_or_connecting --> LOCKED connections_lock_\n"));
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
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::stop_accepting_or_connecting --> RELEASING connections_lock_\n"));
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::stop_accepting_or_connecting --> exit\n"));
}

void
MulticastTransport::passive_connection(MulticastPeer local_peer, MulticastPeer remote_peer)
{


   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::passive_connection --> about to LOCK connections_lock_\n"));

   GuardThreadType guard(this->connections_lock_);

   //### Don't think sending synack is needed due to any call to passive_connection
   //### from datalink or session will have already sent the synack
   //   if(this->connections_.count(peer)){
   //      //need to send ack to active side so it can return from connect_datalink
   //     MulticastSession_rch session =
   //            this->start_session(this->server_link_, peer, false /*!active*/);
   //     if(!session.is_nil()) {
   //        session->send_synack();
   //     }
   //   }

   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::passive_connection --> LOCKED connections_lock_\n"));

  VDBG_LVL((LM_DEBUG, "(%P|%t) MulticastTransport[%C]::passive_connection "
            "from remote peer 0x%x to local peer 0x%x\n",
            this->config_i_->name().c_str(), remote_peer, local_peer), 2);

  const Peers peers(remote_peer, local_peer);
  const PendConnMap::iterator pend = this->pending_connections_.find(peers);

   /*	  if (pend != pending_connections_.end()) {
	    VDBG((LM_DEBUG, "(%P|%t) MulticastTransport::passive_connection completing\n"));
	    const DataLink_rch link = static_rchandle_cast<DataLink>(server_link_);
	    for (size_t i = 0; i < pend->second.size(); ++i) {
	      pend->second[i].first->use_datalink(pend->second[i].second, link);
	    }
	    this->pending_connections_.erase(pend);
	  }
    */
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
         //for (size_t i = 0; i < pend->second.size(); ++i) {
         //for(size_t i=0; i < num_callbacks; ++i) {
         //### Debug statements to track where associate is failing
         if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::passive_connection --> about to call use_datalink in loop\n"));
         if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::passive_connection --> RELEASE connections_lock_ to call use_datalink\n"));
         guard.release();
         //### Debug statements to track where associate is failing
         if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::passive_connection --> RELEASED connections_lock_\n"));
         pend_client->use_datalink(remote_repo, link);
         if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::passive_connection --> trying to LOCK connections_lock_ after use_datalink\n"));

         guard.acquire();

         if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::passive_connection --> LOCKED connections_lock_ after use_datalink\n"));
         //pend->second[i].first->use_datalink(pend->second[i].second, link);
         //### Debug statements to track where associate is failing
         if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::passive_connection --> finished use_datalink in loop\n"));
    } while ((updated_pend = pending_connections_.find(peers)) != pending_connections_.end());
  }
  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::passive_connection --> inserting peer into connections_\n"));
  //if connection was pending, calls to use_datalink finalized the connection
  //if it was not previously pending, accept_datalink() will finalize connection
  this->connections_.insert(peers);
  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:MulticastTransport::passive_connection --> RELEASING connections_lock_\n"));
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
