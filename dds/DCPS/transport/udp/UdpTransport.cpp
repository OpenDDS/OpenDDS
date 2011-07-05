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

namespace {

const CORBA::Long TRANSPORT_INTERFACE_ID(0x4447524D); // DGRM

} // namespace

namespace OpenDDS {
namespace DCPS {

UdpTransport::UdpTransport(const TransportInst_rch& inst)
{
  if (!inst.is_nil()) {
    configure(inst.in());
  }
}

DataLink*
UdpTransport::find_or_create_datalink(
  RepoId /*local_id*/,
  const AssociationData* remote_association,
  CORBA::Long priority,
  bool active)
{
  ACE_INET_Addr remote_address(
    connection_info_i(remote_association->remote_data_));
  bool is_loopback = remote_address == this->config_i_->local_address_;
  PriorityKey key(priority, remote_address, is_loopback, active);

  if (active) {
    UdpDataLinkMap::iterator it(this->client_links_.find(key));
    if (it != this->client_links_.end()) {
      return UdpDataLink_rch(it->second)._retn(); // found
    }

  } else if (!this->server_link_.is_nil()) {
    // A single DataLink is managed for all passive reservations:
    return UdpDataLink_rch(this->server_link_)._retn(); // found
  }

  // Create new DataLink for logical connection:
  UdpDataLink_rch link;
  ACE_NEW_RETURN(link, UdpDataLink(this, active), 0);

  if (link.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpTransport::find_or_create_datalink: ")
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

  if (active) {
    this->client_links_.insert(UdpDataLinkMap::value_type(key, link));
  } else {
    this->server_link_ = link;
  }

  return link._retn();
}

int
UdpTransport::configure_i(TransportInst* config)
{
  this->config_i_ = dynamic_cast<UdpInst*>(config);
  if (this->config_i_ == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     -1);
  }
  this->config_i_->_add_ref();

  this->create_reactor_task();

  return 0;
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

int
UdpTransport::connection_info_i(TransportInterfaceInfo& info) const
{
  NetworkAddress network_address(this->config_i_->local_address_);

  ACE_OutputCDR cdr;
  cdr << network_address;

  CORBA::ULong len = static_cast<CORBA::ULong>(cdr.total_length());
  char *buffer = const_cast<char*>(cdr.buffer()); // safe

  info.transport_id = TRANSPORT_INTERFACE_ID;
  info.data = TransportInterfaceBLOB(len, len,
    reinterpret_cast<CORBA::Octet*>(buffer));

  return 0;
}

ACE_INET_Addr
UdpTransport::connection_info_i(const TransportInterfaceInfo& info) const
{
  if (info.transport_id != TRANSPORT_INTERFACE_ID) {
    ACE_ERROR((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: ")
               ACE_TEXT("UdpTransport::get_connection_info: ")
               ACE_TEXT("transport interface ID does not match: 0x%x\n"),
               info.transport_id));
  }

  ACE_INET_Addr local_address;
  NetworkAddress network_address;

  size_t len = info.data.length();
  const char* buffer = reinterpret_cast<const char*>(info.data.get_buffer());

  ACE_InputCDR cdr(buffer, len);
  cdr >> network_address;

  network_address.to_addr(local_address);

  return local_address;
}

bool
UdpTransport::acked(RepoId /*local_id*/, RepoId /*remote_id*/)
{
  return true;
}

void
UdpTransport::remove_ack(RepoId /*local_id*/, RepoId /*remote_id*/)
{
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

} // namespace DCPS
} // namespace OpenDDS
