/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_UDP_UDPDATALINK_H
#define OPENDDS_DCPS_TRANSPORT_UDP_UDPDATALINK_H

#include "Udp_Export.h"

#include "UdpSendStrategy.h"
#include "UdpSendStrategy_rch.h"
#include "UdpReceiveStrategy.h"
#include "UdpReceiveStrategy_rch.h"
#include "UdpInst_rch.h"

#include "ace/Basic_Types.h"
#include "ace/SOCK_Dgram.h"

#include "dds/DCPS/transport/framework/DataLink.h"
#include "dds/DCPS/ReactorTask.h"
#include "dds/DCPS/ReactorTask_rch.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class UdpTransport;
class ReceivedDataSample;
typedef RcHandle<UdpTransport> UdpTransport_rch;

class OpenDDS_Udp_Export UdpDataLink
  : public DataLink {
public:
  UdpDataLink(UdpTransport& transport,
              Priority   priority,
              const ReactorTask_rch& reactor_task,
              bool          active);

  bool active() const;

  ReactorTask_rch reactor_task();

  ACE_Reactor* get_reactor();

  ACE_INET_Addr& remote_address();

  ACE_SOCK_Dgram& socket();

  bool open(const ACE_INET_Addr& remote_address);

  void control_received(ReceivedDataSample& sample,
                        const ACE_INET_Addr& remote_address);

protected:
  bool active_;

  ReactorTask_rch reactor_task_;

  UdpSendStrategy_rch send_strategy_;
  UdpReceiveStrategy_rch recv_strategy_;

  virtual void stop_i();

private:
  ACE_INET_Addr remote_address_;

  ACE_SOCK_Dgram socket_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
# include "UdpDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_UDPDATALINK_H */
