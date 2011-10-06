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

namespace OpenDDS {
namespace DCPS {

class RtpsUdpDataLink;

class OpenDDS_Rtps_Udp_Export RtpsUdpSendStrategy
  : public TransportSendStrategy {
public:
  explicit RtpsUdpSendStrategy(RtpsUdpDataLink* link);

  virtual void stop_i();

protected:
  virtual ssize_t send_bytes_i(const iovec iov[], int n);

  virtual size_t max_message_size() const
  {
    return UDP_MAX_MESSAGE_SIZE;
  }

private:
  void marshal_transport_header(ACE_Message_Block* mb);

  RtpsUdpDataLink* link_;
  OpenDDS::RTPS::Header rtps_header_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif /* DCPS_RTPSUDPSENDSTRATEGY_H */
