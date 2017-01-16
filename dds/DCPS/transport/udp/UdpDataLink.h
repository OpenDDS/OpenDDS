/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_UDPDATALINK_H
#define DCPS_UDPDATALINK_H

#include "Udp_Export.h"

#include "UdpSendStrategy.h"
#include "UdpSendStrategy_rch.h"
#include "UdpReceiveStrategy.h"
#include "UdpReceiveStrategy_rch.h"

#include "ace/Basic_Types.h"
#include "ace/SOCK_Dgram.h"

#include "dds/DCPS/transport/framework/DataLink.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class UdpInst;
class UdpTransport;
class ReceivedDataSample;
typedef RcHandle<UdpTransport> UdpTransport_rch;

class OpenDDS_Udp_Export UdpDataLink
  : public DataLink {
public:
  UdpDataLink(const UdpTransport_rch& transport,
              Priority   priority,
              bool          active);

  void configure(UdpInst* config,
                 TransportReactorTask* reactor_task);


  bool active() const;

  UdpInst* config();
  TransportReactorTask* reactor_task();

  ACE_Reactor* get_reactor();

  ACE_INET_Addr& remote_address();

  ACE_SOCK_Dgram& socket();

  bool open(const ACE_INET_Addr& remote_address);

  void control_received(ReceivedDataSample& sample,
                        const ACE_INET_Addr& remote_address);

protected:
  bool active_;

  UdpInst* config_;
  TransportReactorTask* reactor_task_;

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
