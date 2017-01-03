/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPACCEPTOR_H
#define OPENDDS_TCPACCEPTOR_H

#include "TcpTransport_rch.h"
#include "TcpConnection.h"
#include "ace/Acceptor.h"
#include "ace/SOCK_Acceptor.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class TcpInst;

class TcpAcceptor : public ACE_Acceptor<TcpConnection,
      ACE_SOCK_ACCEPTOR> {
public:

  TcpAcceptor(const TcpTransport_rch& transport_impl);
  virtual ~TcpAcceptor();

  // Returns a reference that the caller becomes responsible for.
  TcpTransport_rch transport();

  // This causes the Acceptor to drop its reference to the
  // TcpTransport object.
  void transport_shutdown();

  TcpInst_rch get_configuration();

private:

  TcpTransport_rch transport_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_TCPACCEPTOR_H */
