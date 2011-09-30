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

#include "ace/CDR_Base.h"
#include "ace/Log_Msg.h"

#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/AssociationData.h"

#include "dds/DCPS/RTPS/RtpsMessageTypesTypeSupportImpl.h"

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
  ACE_NEW_RETURN(link_, RtpsUdpDataLink(this, local_prefix), 0);

  TransportReactorTask_rch rt = reactor_task();
  link_->configure(config_i_.in(), rt.in());

  RtpsUdpSendStrategy* send_strategy;
  ACE_NEW_RETURN(send_strategy, RtpsUdpSendStrategy(link_.in()), 0);
  link_->send_strategy(send_strategy);

  RtpsUdpReceiveStrategy* recv_strategy;
  ACE_NEW_RETURN(recv_strategy, RtpsUdpReceiveStrategy(link_.in()), 0);
  link_->receive_strategy(recv_strategy);

  if (!link_->open()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::make_datalink: ")
                      ACE_TEXT("failed to open DataLink!\n")),
                     0);
  }

  return RtpsUdpDataLink_rch(link_)._retn();
}

DataLink*
RtpsUdpTransport::find_datalink_i(const RepoId& /*local_id*/,
                                  const RepoId& /*remote_id*/,
                                  const TransportBLOB& /*remote_data*/,
                                  CORBA::Long /*priority*/,
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
                                     CORBA::Long /*priority*/)
{
  RtpsUdpDataLink_rch link;
  if (link_.is_nil()) {
    link = make_datalink(local_id.guidPrefix);
  }

  link->add_locator(remote_id, get_connection_addr(remote_data));
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
                                ce.priority_);
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
  ACE_Data_Block db(remote.length(), ACE_Message_Block::MB_DATA,
    reinterpret_cast<const char*>(remote.get_buffer()),
    0 /*alloc*/, 0 /*lock*/, ACE_Message_Block::DONT_DELETE, 0 /*db_alloc*/);
  ACE_Message_Block mb(&db, ACE_Message_Block::DONT_DELETE, 0 /*mb_alloc*/);

  Serializer ser(&mb, ACE_CDR_BYTE_ORDER, Serializer::ALIGN_CDR);
  LocatorSeq locators;
  if (!(ser >> locators)) {
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
  info.transport_type = "rtps_udp";
  //TODO: Need local address info from sockets / ACE
  //      Marshal in the form of a LocatorSeq (see get_connection_addr() above)
  return true;
}

bool
RtpsUdpTransport::configure_i(TransportInst* config)
{
  this->config_i_ =
    RcHandle<RtpsUdpInst>(dynamic_cast<RtpsUdpInst*>(config), false);

  if (this->config_i_.is_nil()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpTransport::configure_i: ")
                      ACE_TEXT("invalid configuration!\n")),
                     false);
  }

  this->create_reactor_task();

  return true;
}

void
RtpsUdpTransport::shutdown_i()
{
  // Shutdown reserved datalinks and release configuration:

  this->config_i_ = 0;
}

void
RtpsUdpTransport::release_datalink_i(DataLink*, bool /*release_pending*/)
{
  this->link_ = 0;
}

void
RtpsUdpTransport::passive_connection(const ACE_INET_Addr& remote_address,
                                     ACE_Message_Block* data)
{
}

} // namespace DCPS
} // namespace OpenDDS
