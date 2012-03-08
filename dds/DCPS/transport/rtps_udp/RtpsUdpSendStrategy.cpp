/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpSendStrategy.h"
#include "RtpsUdpDataLink.h"
#include "RtpsUdpInst.h"

#include "dds/DCPS/transport/framework/NullSynchStrategy.h"
#include "dds/DCPS/transport/framework/TransportCustomizedElement.h"
#include "dds/DCPS/transport/framework/TransportSendElement.h"

#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/RtpsMessageTypesTypeSupportImpl.h"

#include "dds/DCPS/Serializer.h"

#include <cstring>

namespace OpenDDS {
namespace DCPS {

RtpsUdpSendStrategy::RtpsUdpSendStrategy(RtpsUdpDataLink* link)
  : TransportSendStrategy(TransportInst_rch(link->config(), false),
                          0,  // synch_resource
                          link->transport_priority(),
                          new NullSynchStrategy),
    link_(link),
    override_dest_(0),
    rtps_header_db_(RTPS::RTPSHDR_SZ, ACE_Message_Block::MB_DATA,
                    rtps_header_data_, 0, 0, ACE_Message_Block::DONT_DELETE, 0),
    rtps_header_mb_(&rtps_header_db_, ACE_Message_Block::DONT_DELETE)
{
  rtps_header_.prefix[0] = 'R';
  rtps_header_.prefix[1] = 'T';
  rtps_header_.prefix[2] = 'P';
  rtps_header_.prefix[3] = 'S';
  rtps_header_.version = OpenDDS::RTPS::PROTOCOLVERSION;
  rtps_header_.vendorId = OpenDDS::RTPS::VENDORID_OPENDDS;
  std::memcpy(rtps_header_.guidPrefix, link->local_prefix(),
              sizeof(GuidPrefix_t));
  Serializer writer(&rtps_header_mb_);
  // byte order doesn't matter for the RTPS Header
  writer << rtps_header_;
}

ssize_t
RtpsUdpSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  if (override_dest_) {
    return send_multi_i(iov, n, *override_dest_);
  }

  // determine destination address(es) from TransportQueueElement in progress
  TransportQueueElement* elem = current_packet_first_element();
  if (!elem) {
    errno = ENOTCONN;
    return -1;
  }

  const RepoId remote_id = elem->subscription_id();
  std::set<ACE_INET_Addr> addrs;

  if (remote_id != GUID_UNKNOWN) {
    const ACE_INET_Addr remote = link_->get_locator(remote_id);
    if (remote != ACE_INET_Addr()) {
      addrs.insert(remote);
    }
  }

  if (addrs.empty()) {
    link_->get_locators(elem->publication_id(), addrs);
  }

  if (addrs.empty()) {
    errno = ENOTCONN;
    return -1;
  }

  return send_multi_i(iov, n, addrs);
}

RtpsUdpSendStrategy::OverrideToken
RtpsUdpSendStrategy::override_destinations(const std::set<ACE_INET_Addr>& dest)
{
  override_dest_ = &dest;
  return OverrideToken(this);
}

RtpsUdpSendStrategy::OverrideToken::~OverrideToken()
{
  outer_->override_dest_ = 0;
}

void
RtpsUdpSendStrategy::marshal_transport_header(ACE_Message_Block* mb)
{
  Serializer writer(mb); // byte order doesn't matter for the RTPS Header
  writer.write_octet_array(reinterpret_cast<ACE_CDR::Octet*>(rtps_header_data_),
    RTPS::RTPSHDR_SZ);
}

void
RtpsUdpSendStrategy::send_rtps_control(ACE_Message_Block& submessages,
                                       const std::set<ACE_INET_Addr>& addrs)
{
  rtps_header_mb_.cont(&submessages);

  iovec iov[MAX_SEND_BLOCKS];
  const int num_blocks = mb_to_iov(rtps_header_mb_, iov);
  const ssize_t result = send_multi_i(iov, num_blocks, addrs);
  if (result < 0) {
    ACE_DEBUG((LM_ERROR, "(%P|%t) RtpsUdpSendStrategy::send_rtps_control() - "
      "failed to send RTPS control message\n"));
  }

  rtps_header_mb_.cont(0);
}

ssize_t
RtpsUdpSendStrategy::send_multi_i(const iovec iov[], int n,
                                  const std::set<ACE_INET_Addr>& addrs)
{
  ssize_t result = -1;
  typedef std::set<ACE_INET_Addr>::const_iterator iter_t;
  for (iter_t iter = addrs.begin(); iter != addrs.end(); ++iter) {
#ifdef ACE_HAS_IPV6
    ACE_SOCK_Dgram& sock = link_->socket_for(iter->get_type());
#define USE_SOCKET sock
#else
#define USE_SOCKET link_->unicast_socket()
#endif
    ssize_t result_per_dest = USE_SOCKET.send(iov, n, *iter);
    if (result_per_dest < 0) {
      ACE_TCHAR addr_buff[256] = {};
      iter->addr_to_string(addr_buff, 256, 0);
      ACE_ERROR((LM_ERROR, "(%P|%t) RtpsUdpSendStrategy::send_multi_i() - "
        "destination %s failed %p\n", addr_buff, ACE_TEXT("send")));
    } else {
      result = result_per_dest;
    }
  }
  return result;
}

void
RtpsUdpSendStrategy::stop_i()
{
}

} // namespace DCPS
} // namespace OpenDDS
