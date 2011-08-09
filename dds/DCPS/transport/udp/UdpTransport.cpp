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
#include "dds/DCPS/AssociationData.h"

namespace OpenDDS {
namespace DCPS {

UdpTransport::UdpTransport(const TransportInst_rch& inst)
{
  if (!inst.is_nil()) {
    configure(inst.in());
  }
}

UdpDataLink*
UdpTransport::make_datalink(const ACE_INET_Addr& remote_address, bool active)
{
  UdpDataLink_rch link;
  ACE_NEW_RETURN(link, UdpDataLink(this, active), 0);

  if (link.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpTransport::make_datalink: ")
                      ACE_TEXT("failed to create DataLink!\n")),
                     0);
  }

  // Configure link with transport configuration and reactor task:
  link->configure(this->config_i_.in(), reactor_task());

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
                      ACE_TEXT("UdpTransport::find_or_create_datalink: ")
                      ACE_TEXT("failed to open DataLink!\n")),
                     0);
  }

  return link._retn();
}

DataLink*
UdpTransport::find_datalink_i(const RepoId& /*local_id*/,
                              const RepoId& /*remote_id*/,
                              const TransportBLOB& remote_data,
                              CORBA::Long priority,
                              bool active)
{
  ACE_INET_Addr remote_address = get_connection_addr(remote_data);

  bool is_loopback = remote_address == this->config_i_->local_address_;
  PriorityKey key(priority, remote_address, is_loopback, active);

  if (active) {
    UdpDataLinkMap::iterator it(this->client_links_.find(key));
    if (it != this->client_links_.end()) {
      return UdpDataLink_rch(it->second)._retn(); // found
    }
  } else {
    return UdpDataLink_rch(this->server_link_)._retn(); // found
  }

  return 0;
}

DataLink*
UdpTransport::connect_datalink_i(const RepoId& /*local_id*/,
                                 const RepoId& /*remote_id*/,
                                 const TransportBLOB& remote_data,
                                 CORBA::Long priority)
{
  ACE_INET_Addr remote_address = get_connection_addr(remote_data);
  const bool active = true;

  bool is_loopback = remote_address == this->config_i_->local_address_;
  PriorityKey key(priority, remote_address, is_loopback, active);

  // Create new DataLink for logical connection:
  UdpDataLink_rch link = make_datalink(remote_address, active);
  this->client_links_.insert(UdpDataLinkMap::value_type(key, link));
  return link._retn();
}

DataLink*
UdpTransport::accept_datalink(ConnectionEvent& /*ce*/)
{
  return UdpDataLink_rch(this->server_link_)._retn();
}

void
UdpTransport::stop_accepting(ConnectionEvent& /*ce*/)
{
  // nothing needed here, since accept_datalink doesn't store the event
}

bool
UdpTransport::configure_i(TransportInst* config)
{
  this->config_i_ = dynamic_cast<UdpInst*>(config);
  if (this->config_i_ == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     false);
  }
  this->config_i_->_add_ref();

  this->create_reactor_task();

  // Our "server side" data link is created here, similar to the acceptor_
  // in the TcpTransport implementation.  This establishes a socket as an
  // endpoint that we can advertise to peers via connection_info_i().

  this->server_link_ = make_datalink(ACE_INET_Addr(), false);

  return true;
}

void
UdpTransport::shutdown_i()
{
  // Shutdown reserved datalinks and release configuration:
  for (UdpDataLinkMap::iterator it(this->client_links_.begin());
       it != this->client_links_.end(); ++it) {
    it->second->transport_shutdown();
  }
  this->client_links_.clear();

  this->config_i_ = 0;
}

bool
UdpTransport::connection_info_i(TransportLocator& info) const
{
  NetworkAddress network_address(this->config_i_->local_address_);

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
UdpTransport::release_datalink_i(DataLink* link, bool /*release_pending*/)
{
  for (UdpDataLinkMap::iterator it(this->client_links_.begin());
       it != this->client_links_.end(); ++it) {
    // We are guaranteed to have exactly one matching DataLink
    // in the map; release any resources held and return.
    if (link == static_cast<DataLink*>(it->second.in())) {
      this->client_links_.erase(it);
      return;
    }
  }
}

void
UdpTransport::passive_connection(const ACE_INET_Addr& remote_address,
                                 ACE_Message_Block* data)
{
  //TODO: Demarshal 'data' to recover the remote side's PriorityKey
  //      and update local data structures.

  // Send an ack so that the active side can return from connect_datalink_i().
  // This is just a single byte of arbitrary data, the remote side is not yet
  // using the framework (TransportHeader, DataSampleHeader, ReceiveStrategy).
  const char ack_data = 23;
  this->server_link_->socket().send(&ack_data, 1, remote_address);
}

} // namespace DCPS
} // namespace OpenDDS
