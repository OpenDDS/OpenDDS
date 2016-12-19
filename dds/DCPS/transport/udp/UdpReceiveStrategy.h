/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_UDPRECEIVESTRATEGY_H
#define DCPS_UDPRECEIVESTRATEGY_H

#include "Udp_Export.h"

#include "ace/Event_Handler.h"
#include "ace/INET_Addr.h"

#include "dds/DCPS/transport/framework/TransportReceiveStrategy_T.h"
#include "dds/DCPS/PoolAllocator.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class UdpDataLink;

class OpenDDS_Udp_Export UdpReceiveStrategy
  : public TransportReceiveStrategy<>,
    public ACE_Event_Handler {
public:
  explicit UdpReceiveStrategy(UdpDataLink* link);

  virtual ACE_HANDLE get_handle() const;
  virtual int handle_input(ACE_HANDLE fd);

protected:
  virtual ssize_t receive_bytes(iovec iov[],
                                int n,
                                ACE_INET_Addr& remote_address,
                                ACE_HANDLE fd);

  virtual void deliver_sample(ReceivedDataSample& sample,
                              const ACE_INET_Addr& remote_address);

  virtual int start_i();
  virtual void stop_i();

  virtual bool check_header(const TransportHeader& header);

  virtual bool check_header(const DataSampleHeader& header)
  {
    return TransportReceiveStrategy<>::check_header(header);
  }

private:

  virtual bool reassemble(ReceivedDataSample& data);

  UdpDataLink* link_;
  SequenceNumber expected_;
  ACE_INET_Addr remote_address_; // of the current datagram

  typedef std::pair<TransportReassembly, SequenceNumber> ReassemblyInfo;
  typedef OPENDDS_MAP(ACE_INET_Addr, ReassemblyInfo) ReassemblyMap;
  ReassemblyMap reassembly_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_UDPRECEIVESTRATEGY_H */
