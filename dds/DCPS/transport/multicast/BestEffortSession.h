/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_MULTICAST_BESTEFFORTSESSION_H
#define OPENDDS_DCPS_TRANSPORT_MULTICAST_BESTEFFORTSESSION_H

#include "Multicast_Export.h"

#include "MulticastSession.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export BestEffortSession
  : public MulticastSession {
public:
  BestEffortSession(RcHandle<ReactorInterceptor> interceptor,
                    MulticastDataLink* link,
                    MulticastPeer remote_peer);

  virtual bool check_header(const TransportHeader& header);
  virtual void record_header_received(const TransportHeader& header);

  virtual bool ready_to_deliver(const TransportHeader& header,
                                const ReceivedDataSample& data);

  virtual bool start(bool active, bool acked);
  virtual bool is_reliable() { return false;}

private:
  SequenceNumber expected_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_BESTEFFORTSESSION_H */
