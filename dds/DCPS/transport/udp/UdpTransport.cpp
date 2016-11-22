/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpTransport.h"
#include "UdpInst_rch.h"
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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

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

UdpDataLink_rch
UdpTransport::make_datalink(const ACE_INET_Addr& remote_address,
                            Priority priority, bool active)
{
  UdpDataLink_rch link(new UdpDataLink(this, priority, active));
  // Configure link with transport configuration and reactor task:
  TransportReactorTask_rch rtask (reactor_task());
  link->configure(config_i_.in(), rtask.in());

  // Assign send strategy:
  link->send_strategy(new UdpSendStrategy(link.in()));

  // Assign receive strategy:
  link->receive_strategy(new UdpReceiveStrategy(link.in()));

  // Open logical connection:
  if (link->open(remote_address)) {
    return link;
  }

  ACE_DEBUG((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: ")
              ACE_TEXT("UdpTransport::make_datalink: ")
              ACE_TEXT("failed to open DataLink!\n")));

  return UdpDataLink_rch();
}

TransportImpl::AcceptConnectResult
UdpTransport::connect_datalink(const RemoteTransport& remote,
                               const ConnectionAttribs& attribs,
                               const TransportClient_rch& )
{
  UdpInst_rch tmp_config(this->config_i_.in(), false);
  if (this->is_shut_down() || this->config_i_.is_nil()) {
    return AcceptConnectResult(AcceptConnectResult::ACR_FAILED);
  }
  const ACE_INET_Addr remote_address = get_connection_addr(remote.blob_);
  const bool active = true;
  const PriorityKey key = blob_to_key(remote.blob_, attribs.priority_, tmp_config->local_address(), active);
  tmp_config = 0; //no longer need to hold on to local config
  GuardType guard(client_links_lock_);
  if (this->is_shut_down() || this->config_i_.is_nil()) {
    return AcceptConnectResult(AcceptConnectResult::ACR_FAILED);
  }

  const UdpDataLinkMap::iterator it(client_links_.find(key));
  if (it != client_links_.end()) {
    VDBG((LM_DEBUG, "(%P|%t) UdpTransport::connect_datalink found\n"));
    return AcceptConnectResult(UdpDataLink_rch(it->second)._retn());
  }

  // Create new DataLink for logical connection:
  UdpDataLink_rch link (make_datalink(remote_address,
                                       attribs.priority_,
                                       active));

  if (!link.is_nil()) {
    client_links_.insert(UdpDataLinkMap::value_type(key, link));
    VDBG((LM_DEBUG, "(%P|%t) UdpTransport::connect_datalink connected\n"));
  }

  return AcceptConnectResult(link._retn());
}

TransportImpl::AcceptConnectResult
UdpTransport::accept_datalink(const RemoteTransport& remote,
                              const ConnectionAttribs& attribs,
                              const TransportClient_rch& client)
{
  ACE_Guard<ACE_Recursive_Thread_Mutex> guard(connections_lock_);
  //GuardType guard(connections_lock_);
  const PriorityKey key = blob_to_key(remote.blob_,
                                      attribs.priority_, config_i_->local_address(), false /* !active */);
  if (server_link_keys_.count(key)) {
    VDBG((LM_DEBUG, "(%P|%t) UdpTransport::accept_datalink found\n"));
    return AcceptConnectResult(UdpDataLink_rch(server_link_)._retn());
  }

  else if (pending_server_link_keys_.count(key)) {
    pending_server_link_keys_.erase(key);
    server_link_keys_.insert(key);
    VDBG((LM_DEBUG, "(%P|%t) UdpTransport::accept_datalink completed\n"));
    return AcceptConnectResult(UdpDataLink_rch(server_link_)._retn());
  } else {
    const DataLink::OnStartCallback callback(client, remote.repo_id_);
    pending_connections_[key].push_back(callback);
    VDBG((LM_DEBUG, "(%P|%t) UdpTransport::accept_datalink pending\n"));
    return AcceptConnectResult(AcceptConnectResult::ACR_SUCCESS);
  }
     return AcceptConnectResult();
}

void
UdpTransport::stop_accepting_or_connecting(const TransportClient_rch& client,
                                           const RepoId& remote_id)
{
  VDBG((LM_DEBUG, "(%P|%t) UdpTransport::stop_accepting_or_connecting\n"));

  //GuardType guard(connections_lock_);
  ACE_Guard<ACE_Recursive_Thread_Mutex> guard(connections_lock_);

  for (PendConnMap::iterator it = pending_connections_.begin();
       it != pending_connections_.end(); ++it) {
    for (size_t i = 0; i < it->second.size(); ++i) {
      if (it->second[i].first == client && it->second[i].second == remote_id) {
        it->second.erase(it->second.begin() + i);
        break;
      }
    }
    if (it->second.empty()) {
      pending_connections_.erase(it);
      return;
    }
  }
}

bool
UdpTransport::configure_i(TransportInst* config)
{
  config_i_ = UdpInst_rch(dynamic_cast<UdpInst*>(config), false);

  if (!config_i_) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     false);
  }

  create_reactor_task();

  // Override with DCPSDefaultAddress.
  if (this->config_i_->local_address() == ACE_INET_Addr () &&
      !TheServiceParticipant->default_address ().empty ()) {
    this->config_i_->local_address(0, TheServiceParticipant->default_address ().c_str ());
  }

  // Our "server side" data link is created here, similar to the acceptor_
  // in the TcpTransport implementation.  This establishes a socket as an
  // endpoint that we can advertise to peers via connection_info_i().
  server_link_ = make_datalink(this->config_i_->local_address(), 0 /* priority */, false);
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
  this->config_i_->populate_locator(info);
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
                          ACE_INET_Addr local_addr,
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
  const bool is_loopback = remote_address == local_addr;

  return PriorityKey(priority, remote_address, is_loopback, active);
}

void
UdpTransport::passive_connection(const ACE_INET_Addr& remote_address,
                                 ACE_Message_Block* data)
{
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

  const PriorityKey key = blob_to_key(blob, priority, config_i_->local_address(), false /* passive */);

  ACE_Guard<ACE_Recursive_Thread_Mutex> guard(connections_lock_);

  //GuardType guard(connections_lock_);
  const PendConnMap::iterator pend = pending_connections_.find(key);

  if (pend != pending_connections_.end()) {

     //don't hold connections_lock_ while calling use_datalink
     //guard.release();

    VDBG((LM_DEBUG, "(%P|%t) UdpTransport::passive_connection completing\n"));

    const DataLink_rch link = static_rchandle_cast<DataLink>(server_link_);

    //Insert key now to make sure when releasing guard to call use_datalink
    //if an accept_datalink obtains lock first it will see that it can proceed
    //with using the link and do its own use_datalink call.
    server_link_keys_.insert(key);

    //create a copy of the size of callback vector so that if use_datalink_i -> stop_accepting_or_connecting
    //finds that callbacks vector is empty and deletes pending connection & its callback vector for loop can
    //still exit the loop without checking the size of invalid memory
    //size_t num_callbacks = pend->second.size();

    //Create a copy of the vector of callbacks to process, making sure that each is
    //still present in the actual pending_connections_ before calling use_datalink
    Callbacks tmp(pend->second);
    for (size_t i = 0; i < tmp.size(); ++i) {
      const PendConnMap::iterator pend = pending_connections_.find(key);
      if (pend != pending_connections_.end()) {
        const Callbacks::iterator tmp_iter = find(pend->second.begin(),
                                                  pend->second.end(),
                                                  tmp.at(i));
        if (tmp_iter != pend->second.end()) {
          TransportClient_rch pend_client = tmp.at(i).first;
          RepoId remote_repo = tmp.at(i).second;
          guard.release();
          pend_client->use_datalink(remote_repo, link);
          guard.acquire();
        }
      }
    }
  } else {
    // still hold guard(connections_lock_) at this point so
    // pending_server_link_keys_ is protected for insert

    VDBG((LM_DEBUG, "(%P|%t) UdpTransport::passive_connection pending\n"));
    // accept_datalink() will complete the connection.
    pending_server_link_keys_.insert(key);
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
