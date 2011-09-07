/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RTPSUDPRECEIVESTRATEGY_H
#define DCPS_RTPSUDPRECEIVESTRATEGY_H

#include "Rtps_Udp_Export.h"

#include "ace/Event_Handler.h"

#include "dds/DCPS/transport/framework/TransportReceiveStrategy.h"

namespace OpenDDS {
namespace DCPS {

class RtpsUdpDataLink;

class OpenDDS_Rtps_Udp_Export RtpsUdpReceiveStrategy
  : public TransportReceiveStrategy,
    public ACE_Event_Handler {
public:
  explicit RtpsUdpReceiveStrategy(RtpsUdpDataLink* link);

  virtual ACE_HANDLE get_handle() const;
  virtual int handle_input(ACE_HANDLE fd);

protected:
  virtual ssize_t receive_bytes(iovec iov[],
                                int n,
                                ACE_INET_Addr& remote_address);

  virtual void deliver_sample(ReceivedDataSample& sample,
                              const ACE_INET_Addr& remote_address);

  virtual int start_i();
  virtual void stop_i();

  virtual bool check_header(const TransportHeader& header);

  virtual bool check_header(const DataSampleHeader& header)
  {
    return TransportReceiveStrategy::check_header(header);
  }

private:
  RtpsUdpDataLink* link_;
  SequenceNumber last_received_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RTPSUDPRECEIVESTRATEGY_H */
