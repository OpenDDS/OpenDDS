/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_UDP_UDPSENDSTRATEGY_H
#define OPENDDS_DCPS_TRANSPORT_UDP_UDPSENDSTRATEGY_H

#include "Udp_Export.h"

#include "dds/DCPS/transport/framework/TransportSendStrategy.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class UdpDataLink;
class UdpInst;
typedef RcHandle<UdpDataLink> UdpDataLink_rch;

class OpenDDS_Udp_Export UdpSendStrategy
  : public TransportSendStrategy {
public:
  UdpSendStrategy(UdpDataLink* link);

  virtual void stop_i();

protected:
  virtual ssize_t send_bytes_i(const iovec iov[], int n);

  virtual size_t max_message_size() const
  {
    return UDP_MAX_MESSAGE_SIZE;
  }

private:
  UdpDataLink* link_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* DCPS_UDPSENDSTRATEGY_H */
