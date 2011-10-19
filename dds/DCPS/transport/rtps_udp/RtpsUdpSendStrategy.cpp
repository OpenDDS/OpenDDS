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
    link_(link)
{
  rtps_header_.prefix[0] = 'R';
  rtps_header_.prefix[1] = 'T';
  rtps_header_.prefix[2] = 'P';
  rtps_header_.prefix[3] = 'S';
  rtps_header_.version = OpenDDS::RTPS::PROTOCOLVERSION;
  rtps_header_.vendorId = OpenDDS::RTPS::VENDORID_OPENDDS;
  std::memcpy(rtps_header_.guidPrefix, link->local_prefix(),
              sizeof(GuidPrefix_t));
}

ssize_t
RtpsUdpSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  // determine destination address(es) from TransportQueueElement in progress
  TransportQueueElement* elem = current_packet_first_element();
  if (!elem) {
    errno = ENOTCONN;
    return -1;
  }
  //TODO: also cover the case where we come in from TransportSendBuffer

  std::set<ACE_INET_Addr> addrs;
  link_->get_locators(elem->publication_id(), addrs);
  if (addrs.empty()) {
    errno = ENOTCONN;
    return -1;
  }

  return send_multi_i(iov, n, addrs);
}

void
RtpsUdpSendStrategy::marshal_transport_header(ACE_Message_Block* mb)
{
  Serializer writer(mb); // byte order doesn't matter for the RTPS Header
  writer << rtps_header_;
}

void
RtpsUdpSendStrategy::send_rtps_control(ACE_Message_Block& submessages,
                                       const std::set<ACE_INET_Addr>& addrs)
{
  ACE_Message_Block message(RtpsTransportHeader::max_marshaled_size());
  //TODO: allocators?
  marshal_transport_header(&message);
  message.cont(&submessages);

  iovec iov[MAX_SEND_BLOCKS];
  int num_blocks = mb_to_iov(message, iov);
  ssize_t result = send_multi_i(iov, num_blocks, addrs);
  if (result < 0) {
    ACE_DEBUG((LM_ERROR, "(%P|%t) RtpsUdpSendStrategy::send_rtps_control() - "
      "failed to send RTPS control message\n"));
  }
}

ssize_t
RtpsUdpSendStrategy::send_multi_i(const iovec iov[], int n,
                                  const std::set<ACE_INET_Addr>& addrs)
{
  ssize_t result = -1;
  typedef std::set<ACE_INET_Addr>::const_iterator iter_t;
  for (iter_t iter = addrs.begin(); iter != addrs.end(); ++iter) {
    ssize_t result_per_dest = link_->unicast_socket().send(iov, n, *iter);
    if (result_per_dest < 0) {
      // TODO: log error?
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
