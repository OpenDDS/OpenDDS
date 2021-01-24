/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_TCP_TCPACCEPTOR_H
#define OPENDDS_DCPS_TRANSPORT_TCP_TCPACCEPTOR_H

#include "TcpTransport_rch.h"
#include "TcpConnection.h"
#include "ace/Acceptor.h"
#include "ace/SOCK_Acceptor.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TcpAcceptor : public ACE_Acceptor<TcpConnection, ACE_SOCK_ACCEPTOR> {
public:

  TcpAcceptor(RcHandle<TcpTransport> transport);
  virtual ~TcpAcceptor();

  // Returns a RcHandle which should be checked before use
  RcHandle<TcpTransport> transport();

  // This causes the Acceptor to drop its reference to the TcpTransport
  void transport_shutdown();

private:

  WeakRcHandle<TcpTransport> transport_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_TCPACCEPTOR_H */
