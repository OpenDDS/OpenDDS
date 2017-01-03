/*
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
#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"

#include "dds/DCPS/Serializer.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

RtpsUdpSendStrategy::RtpsUdpSendStrategy(RtpsUdpDataLink* link,
                                         const TransportInst_rch& inst,
                                         const GuidPrefix_t& local_prefix)
  : TransportSendStrategy(0, inst,
                          0,  // synch_resource
                          link->transport_priority(),
                          make_rch<NullSynchStrategy>()),
    link_(link),
    override_dest_(0),
    override_single_dest_(0),
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
  std::memcpy(rtps_header_.guidPrefix, local_prefix,
              sizeof(GuidPrefix_t));
  Serializer writer(&rtps_header_mb_);
  // byte order doesn't matter for the RTPS Header
  writer << rtps_header_;
}

ssize_t
RtpsUdpSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  if (override_single_dest_) {
    return send_single_i(iov, n, *override_single_dest_);
  }

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
  OPENDDS_SET(ACE_INET_Addr) addrs;

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
RtpsUdpSendStrategy::override_destinations(const ACE_INET_Addr& destination)
{
  override_single_dest_ = &destination;
  return OverrideToken(this);
}

RtpsUdpSendStrategy::OverrideToken
RtpsUdpSendStrategy::override_destinations(const OPENDDS_SET(ACE_INET_Addr)& dest)
{
  override_dest_ = &dest;
  return OverrideToken(this);
}

RtpsUdpSendStrategy::OverrideToken::~OverrideToken()
{
  outer_->override_single_dest_ = 0;
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
                                       const ACE_INET_Addr& addr)
{
  rtps_header_mb_.cont(&submessages);

  iovec iov[MAX_SEND_BLOCKS];
  const int num_blocks = mb_to_iov(rtps_header_mb_, iov);
  const ssize_t result = send_single_i(iov, num_blocks, addr);
  if (result < 0) {
    ACE_DEBUG((LM_ERROR, "(%P|%t) RtpsUdpSendStrategy::send_rtps_control() - "
      "failed to send RTPS control message\n"));
  }

  rtps_header_mb_.cont(0);
}

void
RtpsUdpSendStrategy::send_rtps_control(ACE_Message_Block& submessages,
                                       const OPENDDS_SET(ACE_INET_Addr)& addrs)
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
                                  const OPENDDS_SET(ACE_INET_Addr)& addrs)
{
  ssize_t result = -1;
  typedef OPENDDS_SET(ACE_INET_Addr)::const_iterator iter_t;
  for (iter_t iter = addrs.begin(); iter != addrs.end(); ++iter) {
    const ssize_t result_per_dest = send_single_i(iov, n, *iter);
    if (result_per_dest >= 0) {
      result = result_per_dest;
    }
  }
  return result;
}

ssize_t
RtpsUdpSendStrategy::send_single_i(const iovec iov[], int n,
                                   const ACE_INET_Addr& addr)
{
#ifdef ACE_LACKS_SENDMSG
  char buffer[UDP_MAX_MESSAGE_SIZE];
  char *iter = buffer;
  for (int i = 0; i < n; ++i) {
    if (size_t(iter - buffer + iov[i].iov_len) > UDP_MAX_MESSAGE_SIZE) {
      ACE_ERROR((LM_ERROR, "(%P|%t) RtpsUdpSendStrategy::send_single_i() - "
                 "message too large at index %d size %d\n", i, iov[i].iov_len));
      return -1;
    }
    std::memcpy(iter, iov[i].iov_base, iov[i].iov_len);
    iter += iov[i].iov_len;
  }
  const ssize_t result = link_->unicast_socket().send(buffer, iter - buffer, addr);
#else
  const ssize_t result = link_->unicast_socket().send(iov, n, addr);
#endif
  if (result < 0) {
    ACE_TCHAR addr_buff[256] = {};
    int err = errno;
    addr.addr_to_string(addr_buff, 256, 0);
    errno = err;
    ACE_ERROR((LM_ERROR, "(%P|%t) RtpsUdpSendStrategy::send_single_i() - "
      "destination %s failed %p\n", addr_buff, ACE_TEXT("send")));
  }
  return result;
}

void
RtpsUdpSendStrategy::add_delayed_notification(TransportQueueElement* element)
{
  if (!link_->add_delayed_notification(element)) {
    TransportSendStrategy::add_delayed_notification(element);
  }
}

RemoveResult
RtpsUdpSendStrategy::do_remove_sample(const RepoId& pub_id,
  const TransportQueueElement::MatchCriteria& criteria)
{
  link_->do_remove_sample(pub_id, criteria);
  return TransportSendStrategy::do_remove_sample(pub_id, criteria);
}

void
RtpsUdpSendStrategy::stop_i()
{
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
