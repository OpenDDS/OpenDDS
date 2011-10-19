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

#include "dds/DCPS/RTPS/RtpsMessageTypesC.h"

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

  void send_rtps_control(ACE_Message_Block& submessages,
                         const std::set<ACE_INET_Addr>& destinations);

protected:
  virtual ssize_t send_bytes_i(const iovec iov[], int n);

  virtual size_t max_message_size() const
  {
    // In the initial implementation we are not generating fragments
    return 0; //UDP_MAX_MESSAGE_SIZE;
  }

private:
  void marshal_transport_header(ACE_Message_Block* mb);
  ssize_t send_multi_i(const iovec iov[], int n,
                       const std::set<ACE_INET_Addr>& addrs);

  RtpsUdpDataLink* link_;
  OpenDDS::RTPS::Header rtps_header_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* DCPS_RTPSUDPSENDSTRATEGY_H */
