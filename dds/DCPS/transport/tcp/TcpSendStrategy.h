/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPSENDSTRATEGY_H
#define OPENDDS_TCPSENDSTRATEGY_H

#include "TcpConnection_rch.h"
#include "TcpDataLink_rch.h"
#include "dds/DCPS/transport/framework/TransportSendStrategy.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"

namespace OpenDDS {
namespace DCPS {

class TcpConnection;
class TcpInst;
class TcpSynchResource;

class TcpSendStrategy : public TransportSendStrategy {
public:

  TcpSendStrategy(TcpDataLink*      link,
                        TcpInst* config,
                        TcpConnection*    connection,
                        TcpSynchResource* synch_resource,
                        TransportReactorTask*   task,
                        CORBA::Long             priority);
  virtual ~TcpSendStrategy();

  /// This is called by the datalink object to associate with the "new" connection object.
  /// The "old" connection object is unregistered with the reactor and the "new" connection
  /// object is registered for sending. The implementation of this method is borrowed from
  /// the ReceiveStrategy.
  int reset(TcpConnection* connection);

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

#endif  /* OPENDDS_TCPSENDSTRATEGY_H */
