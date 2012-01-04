/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpTransport.h"
#include "RtpsUdpInst.h"
#include "RtpsUdpSendStrategy.h"
#include "RtpsUdpReceiveStrategy.h"

#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/AssociationData.h"

#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include "dds/DCPS/RTPS/RtpsMessageTypesTypeSupportImpl.h"

#include "ace/CDR_Base.h"
#include "ace/Log_Msg.h"
#include "ace/Sock_Connect.h"


namespace OpenDDS {
namespace DCPS {

RtpsUdpTransport::RtpsUdpTransport(const TransportInst_rch& inst)
{
  if (!inst.is_nil()) {
    configure(inst.in());
  }
}

RtpsUdpDataLink*
RtpsUdpTransport::make_datalink(const GuidPrefix_t& local_prefix)
{
  TransportReactorTask_rch rt = reactor_task();
  ACE_NEW_RETURN(link_,
                 RtpsUdpDataLink(this, local_prefix, config_i_.in(), rt.in()),
                 0);

  RtpsUdpSendStrategy* send_strategy;
  ACE_NEW_RETURN(send_strategy, RtpsUdpSendStrategy(link_.in()), 0);
  link_->send_strategy(send_strategy);

  RtpsUdpReceiveStrategy* recv_strategy;
  ACE_NEW_RETURN(recv_strategy, RtpsUdpReceiveStrategy(link_.in()), 0);
  link_->receive_strategy(recv_strategy);

  if (!link_->open(unicast_socket_)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::make_datalink: ")
                      ACE_TEXT("failed to open DataLink for socket %d\n"),
                      unicast_socket_),
                     0);
  }

  // RtpsUdpDataLink now owns the socket
  unicast_socket_.set_handle(ACE_INVALID_HANDLE);

  return RtpsUdpDataLink_rch(link_)._retn();
}

DataLink*
RtpsUdpTransport::find_datalink_i(const RepoId& /*local_id*/,
                                  const RepoId& /*remote_id*/,
                                  const TransportBLOB& /*remote_data*/,
                                  const ConnectionAttribs& /*attribs*/,
                                  bool /*active*/)
{
  // We're not going to use find_datalink_i() for this transport.
  // Instead, each new association will use either connect or accept.
  return 0;
}

DataLink*
RtpsUdpTransport::connect_datalink_i(const RepoId& local_id,
                                     const RepoId& remote_id,
                                     const TransportBLOB& remote_data,
                                     const ConnectionAttribs& attribs)
{
  RtpsUdpDataLink_rch link = link_;
  if (link_.is_nil()) {
    link = make_datalink(local_id.guidPrefix);
  }

  link->add_locator(remote_id, get_connection_addr(remote_data));
  link->associated(local_id, remote_id, attribs.reliable_);
  return link._retn();
}

DataLink*
RtpsUdpTransport::accept_datalink(ConnectionEvent& ce)
{
  const std::string ttype = "rtps_udp";
  const CORBA::ULong num_blobs = ce.remote_association_.remote_data_.length();

  for (CORBA::ULong idx = 0; idx < num_blobs; ++idx) {
    if (ce.remote_association_.remote_data_[idx].transport_type.in() == ttype) {
      return connect_datalink_i(ce.local_id_, ce.remote_association_.remote_id_,
                                ce.remote_association_.remote_data_[idx].data,
                                ce.attribs_);
    }
  }

  return 0;
}

void
RtpsUdpTransport::stop_accepting(ConnectionEvent& /*ce*/)
{
  // nothing to do here, we don't defer any accept actions in accept_datalink()
}

ACE_INET_Addr
RtpsUdpTransport::get_connection_addr(const TransportBLOB& remote) const
{
  using namespace OpenDDS::RTPS;
  LocatorSeq locators;
  DDS::ReturnCode_t result = blob_to_locators(remote, locators);
  if (result != DDS::RETCODE_OK) {
    return ACE_INET_Addr();
  }

  for (CORBA::ULong i = 0; i < locators.length(); ++i) {
    ACE_INET_Addr addr;
    switch (locators[i].kind) {
#ifdef ACE_HAS_IPV6
    case LOCATOR_KIND_UDPv6:
      addr.set_type(AF_INET6);
      if (addr.set_address(reinterpret_cast<const char*>(locators[i].address),
                           16) == -1) {
        break;
      }
      addr.set_port_number(locators[i].port);
      if (!addr.is_multicast() || config_i_->use_multicast_) {
        return addr;
      }
      break;
#endif
    case LOCATOR_KIND_UDPv4:
      addr.set_type(AF_INET);
      if (addr.set_address(reinterpret_cast<const char*>(locators[i].address)
                           + 12, 4, 0 /*network order*/) == -1) {
        break;
      }
      addr.set_port_number(locators[i].port);
      if (!addr.is_multicast() || config_i_->use_multicast_) {
        return addr;
      }
      break;
    default:
      break;
    }
  }

  return ACE_INET_Addr();
}

bool
RtpsUdpTransport::connection_info_i(TransportLocator& info) const
{
  using namespace OpenDDS::RTPS;

  LocatorSeq locators;
  CORBA::ULong idx = 0;

  // multicast first so it's preferred by remote peers
  if (config_i_->use_multicast_) {
    locators.length(2);
    locators[0].kind =
      (config_i_->multicast_group_address_.get_type() == AF_INET6)
      ? LOCATOR_KIND_UDPv6 : LOCATOR_KIND_UDPv4;
    locators[0].port = config_i_->multicast_group_address_.get_port_number();
    RTPS::address_to_bytes(locators[0].address, 
                           config_i_->multicast_group_address_);
    idx = 1;

  } else {
    locators.length(1);
  }

  locators[idx].kind = (config_i_->local_address_.get_type() == AF_INET6)
                       ? LOCATOR_KIND_UDPv6 : LOCATOR_KIND_UDPv4;
  locators[idx].port = config_i_->local_address_.get_port_number();
  RTPS::address_to_bytes(locators[idx].address, 
                         config_i_->local_address_);

  size_t size = 0, padding = 0;
  gen_find_size(locators, size, padding);
  ACE_Message_Block mb(size + padding);

  Serializer ser(&mb, ACE_CDR_BYTE_ORDER, Serializer::ALIGN_CDR);
  if (!(ser << locators)) {
    return false;
  }

  info.transport_type = "rtps_udp";
  info.data.replace(static_cast<CORBA::ULong>(mb.length()), &mb);
  return true;
}

bool
RtpsUdpTransport::configure_i(TransportInst* config)
{
  config_i_ = RcHandle<RtpsUdpInst>(dynamic_cast<RtpsUdpInst*>(config), false);

  if (config_i_.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     false);
  }

  // Open the socket here so that any addresses/ports left
  // unspecified in the RtpsUdpInst are known by the time we get to
  // connection_info_i().  Opening the sockets here also allows us to
  // detect and report errors during DataReader/Writer setup instead
  // of during association.

  if (unicast_socket_.open(config_i_->local_address_) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::configure_i: socket open:")
                      ACE_TEXT("%m\n")),
                     false);
  }

  if (config_i_->local_address_.is_any()) {

    size_t count;
    ACE_INET_Addr* addrs_raw = 0;
    const int res = ACE::get_ip_interfaces(count, addrs_raw);
    ACE_Auto_Array_Ptr<ACE_INET_Addr> addrs(addrs_raw);
    if (res != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: RtpsUdpDataLink::configure_i - %p\n"),
        ACE_TEXT("ACE::get_ip_interfaces")), false);
    }

    for (int i = 0; i < count; ++i) {
      if (!addrs[i].is_loopback()) {
        config_i_->local_address_ = addrs[i];
      }
    }

    // if it's still "any" at this point, we may only have loopback interface
    if (config_i_->local_address_.is_any() && count > 0) {
      config_i_->local_address_ = addrs[0];
    }
  } 

  if (config_i_->local_address_.get_port_number() == 0) {

    ACE_INET_Addr address;
    if (unicast_socket_.get_local_addr(address) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: RtpsUdpDataLink::configure_i - %p\n"),
        ACE_TEXT("cannot get local addr")), false);
    }
    config_i_->local_address_.set_port_number(address.get_port_number());
  }

  create_reactor_task();

  return true;
}

void
RtpsUdpTransport::shutdown_i()
{
  if (!link_.is_nil()) {
    link_->transport_shutdown();
  }
  link_ = 0;
  config_i_ = 0;
}

void
RtpsUdpTransport::release_datalink_i(DataLink*, bool /*release_pending*/)
{
  link_ = 0;
}


} // namespace DCPS
} // namespace OpenDDS
