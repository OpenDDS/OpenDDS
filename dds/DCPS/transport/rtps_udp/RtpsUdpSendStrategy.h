/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RTPSUDPSENDSTRATEGY_H
#define DCPS_RTPSUDPSENDSTRATEGY_H

#include "Rtps_Udp_Export.h"

#include "dds/DCPS/transport/framework/TransportSendStrategy.h"

#include "dds/DCPS/RTPS/MessageTypes.h"

#include "ace/INET_Addr.h"

#include <set>

namespace OpenDDS {
namespace DCPS {

class RtpsUdpDataLink;

class OpenDDS_Rtps_Udp_Export RtpsUdpSendStrategy
  : public TransportSendStrategy {
public:
  explicit RtpsUdpSendStrategy(RtpsUdpDataLink* link);

  virtual void stop_i();

  struct OverrideToken {
    explicit OverrideToken(RtpsUdpSendStrategy* outer) : outer_(outer) {}
    ~OverrideToken();
    RtpsUdpSendStrategy* outer_;
  };
  friend struct OverrideToken;

  OverrideToken override_destinations(const ACE_INET_Addr& destination);
  OverrideToken override_destinations(
    const std::set<ACE_INET_Addr>& destinations);

  void send_rtps_control(ACE_Message_Block& submessages,
                         const ACE_INET_Addr& destination);
  void send_rtps_control(ACE_Message_Block& submessages,
                         const std::set<ACE_INET_Addr>& destinations);

  static const size_t MAX_MSG_SIZE = UDP_MAX_MESSAGE_SIZE;

protected:
  virtual ssize_t send_bytes_i(const iovec iov[], int n);

  virtual size_t max_message_size() const
  {
    return MAX_MSG_SIZE;
  }

private:
  void marshal_transport_header(ACE_Message_Block* mb);
  ssize_t send_multi_i(const iovec iov[], int n,
                       const std::set<ACE_INET_Addr>& addrs);
  ssize_t send_single_i(const iovec iov[], int n,
                        const ACE_INET_Addr& addr);

  RtpsUdpDataLink* link_;
  const std::set<ACE_INET_Addr>* override_dest_;
  const ACE_INET_Addr* override_single_dest_;

  OpenDDS::RTPS::Header rtps_header_;
  char rtps_header_data_[RTPS::RTPSHDR_SZ];
  ACE_Data_Block rtps_header_db_;
  ACE_Message_Block rtps_header_mb_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* DCPS_RTPSUDPSENDSTRATEGY_H */
