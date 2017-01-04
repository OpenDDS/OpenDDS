/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPRECEIVESTRATEGY_H
#define OPENDDS_TCPRECEIVESTRATEGY_H

#include "TcpConnection_rch.h"
#include "TcpDataLink_rch.h"
#include "dds/DCPS/transport/framework/TransportReceiveStrategy_T.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TcpConnection;

class TcpReceiveStrategy : public TransportReceiveStrategy<> {
public:

  TcpReceiveStrategy(const TcpDataLink_rch& link,
                     const TcpConnection_rch& connection,
                     const TransportReactorTask_rch& task);

  virtual ~TcpReceiveStrategy();

  int reset(const TcpConnection_rch& connection);

  ACE_Reactor* get_reactor();

  bool gracefully_disconnected();

protected:

  virtual ssize_t receive_bytes(iovec iov[],
                                int   n,
                                ACE_INET_Addr& remote_address,
                                ACE_HANDLE fd);

  virtual void deliver_sample(ReceivedDataSample& sample,
                              const ACE_INET_Addr& remote_address);

  virtual int start_i();
  virtual void stop_i();

  // Delegate to the connection object to re-establishment
  // the connection.
  virtual void relink(bool do_suspend = true);

private:

  TcpDataLink_rch    link_;
  TcpConnection_rch  connection_;
  TransportReactorTask_rch reactor_task_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TcpReceiveStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_TCPRECEIVESTRATEGY_H */
