/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpTransport.h"

#include "ace/CDR_Base.h"
#include "ace/Log_Msg.h"

#include "dds/DCPS/transport/framework/NetworkAddress.h"

namespace {

const CORBA::Long TRANSPORT_INTERFACE_ID(0x4447524D); // DGRM

} // namespace

namespace OpenDDS {
namespace DCPS {

DataLink*
UdpTransport::find_or_create_datalink(
  RepoId /*local_id*/,
  const AssociationData* remote_association,
  CORBA::Long /*priority*/,
  bool /*active*/)
{
  RepoId remote_id(remote_association->remote_id_);

  UdpDataLinkMap::iterator it(this->links_.find(remote_id));
  if (it != this->links_.end()) return it->second.in(); // found

  // Create new DataLink for logical connection:
  UdpDataLink_rch link = new UdpDataLink(this);
  if (link.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpTransport::find_or_create_datalink: ")
                      ACE_TEXT("failed to create DataLink!\n")),
                     0);
  }

  // Configure link with transport configuration and reactor task:
  link->configure(this->config_i_.in(), reactor_task());

  // Assign send/receive strategies:
  link->send_strategy(new UdpSendStrategy(link.in()));
  link->receive_strategy(new UdpReceiveStrategy(link.in()));

  ACE_INET_Addr remote_address(
    connection_info_i(remote_association->remote_data_));

  if (!link->open(remote_address)) {
    ACE_TCHAR remote_address_s[64];
    remote_address.addr_to_string(remote_address_s, sizeof(remote_address_s));
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpTransport::find_or_create_datalink: ")
                      ACE_TEXT("failed to open remote address: %C!\n"),
                      remote_address_s),
                     0);
  }

  this->links_.insert(UdpDataLinkMap::value_type(remote_id, link));

  return link._retn();
}

int
UdpTransport::configure_i(TransportConfiguration* config)
{
  this->config_i_ = dynamic_cast<UdpConfiguration*>(config);
  if (this->config_i_.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     -1);
  }
  this->config_i_->_add_ref();  // take ownership

  return 0;
}

void
UdpTransport::shutdown_i()
{
  // Shutdown reserved datalinks and release configuration:
  for (UdpDataLinkMap::iterator it(this->links_.begin());
       it != this->links_.end(); ++it) {
    it->second->transport_shutdown();
  }
  this->links_.clear();

  this->config_i_ = 0;  // release ownership
}

int
UdpTransport::connection_info_i(TransportInterfaceInfo& info) const
{
  NetworkAddress network_address(this->config_i_->local_address_);

  ACE_OutputCDR cdr;
  cdr << network_address;

  size_t len = cdr.total_length();
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
  for (UdpDataLinkMap::iterator it(this->links_.begin());
       it != this->links_.end(); ++it) {
    // We are guaranteed to have exactly one matching DataLink
    // in the map; release any resources held and return.
    if (link == static_cast<DataLink*>(it->second.in())) {
      this->links_.erase(it);
      return;
    }
  }
}

} // namespace DCPS
} // namespace OpenDDS
