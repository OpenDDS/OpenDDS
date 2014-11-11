/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpTransport.h"
#include "UdpInst.h"
#include "UdpSendStrategy.h"
#include "UdpReceiveStrategy.h"

#include "ace/CDR_Base.h"
#include "ace/Log_Msg.h"

#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/transport/framework/PriorityKey.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/AssociationData.h"
//### just for log messages
#include "dds/DCPS/GuidConverter.h"

#include "dds/DCPS/async_debug.h"

namespace OpenDDS {
namespace DCPS {

UdpTransport::UdpTransport(const TransportInst_rch& inst)
{
  if (!inst.is_nil()) {
    if (!configure(inst.in())) {
      throw Transport::UnableToCreate();
    }
  }
}

UdpDataLink*
UdpTransport::make_datalink(const ACE_INET_Addr& remote_address,
                            Priority priority, bool active)
{
  UdpDataLink_rch link;
  ACE_NEW_RETURN(link, UdpDataLink(this, priority, active), 0);

  if (link.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpTransport::make_datalink: ")
                      ACE_TEXT("failed to create DataLink!\n")),
                     0);
  }

  // Configure link with transport configuration and reactor task:
  TransportReactorTask_rch rtask = reactor_task();
  link->configure(config_i_.in(), rtask.in());

  // Assign send strategy:
  UdpSendStrategy* send_strategy;
  ACE_NEW_RETURN(send_strategy, UdpSendStrategy(link.in()), 0);
  link->send_strategy(send_strategy);

  // Assign receive strategy:
  UdpReceiveStrategy* recv_strategy;
  ACE_NEW_RETURN(recv_strategy, UdpReceiveStrategy(link.in()), 0);
  link->receive_strategy(recv_strategy);

  // Open logical connection:
  if (!link->open(remote_address)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpTransport::make_datalink: ")
                      ACE_TEXT("failed to open DataLink!\n")),
                     0);
  }

  return link._retn();
}

TransportImpl::AcceptConnectResult
UdpTransport::connect_datalink(const RemoteTransport& remote,
                               const ConnectionAttribs& attribs,
                               TransportClient* )
{
  const ACE_INET_Addr remote_address = get_connection_addr(remote.blob_);
  const bool active = true;
  const PriorityKey key = blob_to_key(remote.blob_, attribs.priority_, active);
  GuardType guard(client_links_lock_);

  const UdpDataLinkMap::iterator it(client_links_.find(key));
  if (it != client_links_.end()) {
    VDBG((LM_DEBUG, "(%P|%t) UdpTransport::connect_datalink found\n"));
    return AcceptConnectResult(UdpDataLink_rch(it->second)._retn());
  }

  // Create new DataLink for logical connection:
  UdpDataLink_rch link = make_datalink(remote_address,
                                       attribs.priority_,
                                       active);
  client_links_.insert(UdpDataLinkMap::value_type(key, link));

  VDBG((LM_DEBUG, "(%P|%t) UdpTransport::connect_datalink connected\n"));
  return AcceptConnectResult(link._retn());
}

TransportImpl::AcceptConnectResult
UdpTransport::accept_datalink(const RemoteTransport& remote,
                              const ConnectionAttribs& attribs,
                              TransportClient* client)
{
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::accept_datalink --> enter\n"));
   ACE_Guard<ACE_Recursive_Thread_Mutex> guard(connections_lock_);
  //GuardType guard(connections_lock_);
  const PriorityKey key = blob_to_key(remote.blob_,
                                      attribs.priority_, false /* !active */);
  if (server_link_keys_.count(key)) {
     //### Debug statements to track where associate is failing
        if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::accept_datalink --> found server_link_key so return link\n"));
    VDBG((LM_DEBUG, "(%P|%t) UdpTransport::accept_datalink found\n"));
    return AcceptConnectResult(UdpDataLink_rch(server_link_)._retn());
  }

  else if (pending_server_link_keys_.count(key)) {
     //### Debug statements to track where associate is failing
        if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::accept_datalink --> found pending_server_link_key move to server_link_key and return link\n"));
    pending_server_link_keys_.erase(key);
    server_link_keys_.insert(key);
    VDBG((LM_DEBUG, "(%P|%t) UdpTransport::accept_datalink completed\n"));
    return AcceptConnectResult(UdpDataLink_rch(server_link_)._retn());
  } else {
     //### Debug statements to track where associate is failing
        if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::accept_datalink --> adding on start callback\n"));
    const DataLink::OnStartCallback callback(client, remote.repo_id_);
    pending_connections_[key].push_back(callback);
    VDBG((LM_DEBUG, "(%P|%t) UdpTransport::accept_datalink pending\n"));
    return AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS);
  }
     return AcceptConnectResult();
}

void
UdpTransport::stop_accepting_or_connecting(TransportClient* client,
                                           const RepoId& remote_id)
{
   //### Debug statements to track where associate is failing
   GuidConverter remote(remote_id);
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::stop_accepting_or_connecting --> enter for remote_id: %C\n", std::string(remote).c_str()));

  VDBG((LM_DEBUG, "(%P|%t) UdpTransport::stop_accepting_or_connecting\n"));

  //GuardType guard(connections_lock_);
  ACE_Guard<ACE_Recursive_Thread_Mutex> guard(connections_lock_);

  for (PendConnMap::iterator it = pending_connections_.begin();
       it != pending_connections_.end(); ++it) {
    for (size_t i = 0; i < it->second.size(); ++i) {
      if (it->second[i].first == client && it->second[i].second == remote_id) {
         //### Debug statements to track where associate is failing
         GuidConverter remote2(remote_id);
         if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::stop_accepting_or_connecting --> about to erase callback for remote_id: %C\n", std::string(remote2).c_str()));
        it->second.erase(it->second.begin() + i);
        //### Debug statements to track where associate is failing
        GuidConverter remote3(remote_id);
        if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::stop_accepting_or_connecting --> after erase callback remote_id is now: %C\n", std::string(remote3).c_str()));
        break;
      }
    }
    if (it->second.empty()) {
      pending_connections_.erase(it);
      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::stop_accepting_or_connecting --> exit after erasing pending_connection\n"));
      return;
    }
  }
  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::stop_accepting_or_connecting --> exit\n"));
}

bool
UdpTransport::configure_i(TransportInst* config)
{
  config_i_ = dynamic_cast<UdpInst*>(config);
  if (config_i_ == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     false);
  }
  config_i_->_add_ref();

  create_reactor_task();

  // Our "server side" data link is created here, similar to the acceptor_
  // in the TcpTransport implementation.  This establishes a socket as an
  // endpoint that we can advertise to peers via connection_info_i().
  server_link_ = make_datalink(ACE_INET_Addr(), 0 /* priority */, false);
  return true;
}

void
UdpTransport::shutdown_i()
{
  // Shutdown reserved datalinks and release configuration:
  GuardType guard(client_links_lock_);
  for (UdpDataLinkMap::iterator it(client_links_.begin());
       it != client_links_.end(); ++it) {
    it->second->transport_shutdown();
  }
  client_links_.clear();

  server_link_->transport_shutdown();
  server_link_ = 0;

  config_i_ = 0;
}

bool
UdpTransport::connection_info_i(TransportLocator& info) const
{
  NetworkAddress network_address(config_i_->local_address_,
                                 config_i_->local_address_.is_any());
  ACE_OutputCDR cdr;
  cdr << network_address;

  const CORBA::ULong len = static_cast<CORBA::ULong>(cdr.total_length());
  char* buffer = const_cast<char*>(cdr.buffer()); // safe

  info.transport_type = "udp";
  info.data = TransportBLOB(len, len, reinterpret_cast<CORBA::Octet*>(buffer));

  return true;
}

ACE_INET_Addr
UdpTransport::get_connection_addr(const TransportBLOB& data) const
{
  ACE_INET_Addr local_address;
  NetworkAddress network_address;

  size_t len = data.length();
  const char* buffer = reinterpret_cast<const char*>(data.get_buffer());

  ACE_InputCDR cdr(buffer, len);
  cdr >> network_address;

  network_address.to_addr(local_address);

  return local_address;
}

void
UdpTransport::release_datalink(DataLink* link)
{
  GuardType guard(client_links_lock_);
  for (UdpDataLinkMap::iterator it(client_links_.begin());
       it != client_links_.end(); ++it) {
    // We are guaranteed to have exactly one matching DataLink
    // in the map; release any resources held and return.
    if (link == static_cast<DataLink*>(it->second.in())) {
      link->stop();
      client_links_.erase(it);
      return;
    }
  }
}

PriorityKey
UdpTransport::blob_to_key(const TransportBLOB& remote,
                          Priority priority,
                          bool active)
{
  NetworkAddress network_order_address;
  ACE_InputCDR cdr((const char*)remote.get_buffer(), remote.length());

  if ((cdr >> network_order_address) == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: UdpTransport::blob_to_key")
               ACE_TEXT(" failed to de-serialize the NetworkAddress\n")));
  }

  ACE_INET_Addr remote_address;
  network_order_address.to_addr(remote_address);
  const bool is_loopback = remote_address == config_i_->local_address_;

  return PriorityKey(priority, remote_address, is_loopback, active);
}

void
UdpTransport::passive_connection(const ACE_INET_Addr& remote_address,
                                 ACE_Message_Block* data)
{
   //### Debug statements to track where associate is failing
   if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::passive_connection --> enter\n"));

  CORBA::ULong octet_size =
    static_cast<CORBA::ULong>(data->length() - sizeof(Priority));
  Priority priority;
  Serializer serializer(data);
  serializer >> priority;
  TransportBLOB blob(octet_size);
  blob.length(octet_size);
  serializer.read_octet_array(blob.get_buffer(), octet_size);

  // Send an ack so that the active side can return from
  // connect_datalink_i().  This is just a single byte of
  // arbitrary data, the remote side is not yet using the
  // framework (TransportHeader, DataSampleHeader,
  // ReceiveStrategy).
  const char ack_data = 23;
  server_link_->socket().send(&ack_data, 1, remote_address);

  const PriorityKey key = blob_to_key(blob, priority, false /* passive */);

  ACE_Guard<ACE_Recursive_Thread_Mutex> guard(connections_lock_);

  //GuardType guard(connections_lock_);
  const PendConnMap::iterator pend = pending_connections_.find(key);

  if (pend != pending_connections_.end()) {

     //don't hold connections_lock_ while calling use_datalink
     //guard.release();

    VDBG((LM_DEBUG, "(%P|%t) UdpTransport::passive_connection completing\n"));

    //### Debug statements to track where associate is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::passive_connection --> found pending_connection to complete with use_datalink call\n"));

    const DataLink_rch link = static_rchandle_cast<DataLink>(server_link_);

    //### Debug statements to track where associate is failing
    if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::passive_connection --> got pointer to shared data link\n"));

    //create a copy of the size of callback vector so that if use_datalink_i -> stop_accepting_or_connecting
    //finds that callbacks vector is empty and deletes pending connection & its callback vector for loop can
    //still exit the loop without checking the size of invalid memory
    //size_t num_callbacks = pend->second.size();

    //### Debug statements to track where associate is failing
       if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::passive_connection --> this pending connection has %d callbacks and server_link_ is: %s \n", pend->second.size(), server_link_.is_nil() ? "NIL" : "NOT NIL"));
    PendConnMap::iterator updated_pend = pend;
    do {
       TransportClient* pend_client = updated_pend->second.front().first;
       RepoId remote_repo = updated_pend->second.front().second;
    //for (size_t i = 0; i < pend->second.size(); ++i) {
    //for(size_t i=0; i < num_callbacks; ++i) {
       //### Debug statements to track where associate is failing
       if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::passive_connection --> about to call use_datalink in loop\n"));
       guard.release();
       pend_client->use_datalink(remote_repo, link);
       guard.acquire();
      //pend->second[i].first->use_datalink(pend->second[i].second, link);
      //### Debug statements to track where associate is failing
      if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::passive_connection --> finished use_datalink in loop\n"));
    } while ((updated_pend = pending_connections_.find(key)) != pending_connections_.end());
    //### don't need to erase pending_connection here because stop_accepting_or_connecting
    //### will take care of the clean up when appropriate (called from use_datalink above)
    //### this allows no duplicate erase of pend, allowing connections_lock_ to be recursive



       //need to protect server_link_keys_ access below
    //ACE_Guard<ACE_Recursive_Thread_Mutex> guard(connections_lock_);


       //### Debug statements to track where associate is failing
       if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::passive_connection --> inserting key into server_link_keys\n"));
    //Delegate deletion of pending connection to stop_accepting_or_connecting called from use_datalink_i after link known to be valid
    //pending_connections_.erase(pend);
    server_link_keys_.insert(key);


  } else {
     //still hold guard(connections_lock_) at this point so pending_server_link_keys_ is protected for insert
     //### Debug statements to track where associate is failing
     if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::passive_connection --> inserting key into pending_server_link_keys\n"));

    VDBG((LM_DEBUG, "(%P|%t) UdpTransport::passive_connection pending\n"));
    // accept_datalink() will complete the connection.
    pending_server_link_keys_.insert(key);
  }
  //### Debug statements to track where associate is failing
  if (ASYNC_debug) ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ASYNC_DBG:UdpTransport::passive_connection --> exit\n"));
}

} // namespace DCPS
} // namespace OpenDDS
