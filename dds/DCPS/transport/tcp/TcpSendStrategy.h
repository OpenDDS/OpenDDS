/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPSENDSTRATEGY_H
#define OPENDDS_TCPSENDSTRATEGY_H

#include "TcpConnection_rch.h"
#include "TcpDataLink_rch.h"
#include "TcpInst_rch.h"
#include "TcpConnection_rch.h"
#include "dds/DCPS/transport/framework/TransportSendStrategy.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TcpSynchResource;

class TcpSendStrategy : public TransportSendStrategy {
public:

  TcpSendStrategy(std::size_t id,
                  const TcpDataLink_rch& link,
                  const TcpInst_rch& config,
                  const TcpConnection_rch& connection,
                  TcpSynchResource* synch_resource,
                  const TransportReactorTask_rch& task,
                  Priority priority);
  virtual ~TcpSendStrategy();

  /// This is called by the datalink object to associate with the "new" connection object.
  /// The "old" connection object is unregistered with the reactor and the "new" connection
  /// object is registered for sending. The implementation of this method is borrowed from
  /// the ReceiveStrategy.
  int reset(const TcpConnection_rch& connection, bool reset_mode = false);

  /// Enable or disable output processing by the reactor according to mode.
  virtual void schedule_output();

protected:

  virtual ssize_t send_bytes(const iovec iov[], int n, int& bp);
  virtual ACE_HANDLE get_handle();
  virtual ssize_t send_bytes_i(const iovec iov[], int n);

  /// Delegate to the connection object to re-establish
  /// the connection.
  virtual void relink(bool do_suspend = true);

  virtual void stop_i();

private:
  TcpConnection_rch connection_;
  TcpDataLink_rch   link_;
  TransportReactorTask_rch reactor_task_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_TCPSENDSTRATEGY_H */
