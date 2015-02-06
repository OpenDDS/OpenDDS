/*
 * $Id$
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

namespace OpenDDS {
namespace DCPS {

class TcpInst;

class TcpAcceptor : public ACE_Acceptor<TcpConnection,
      ACE_SOCK_ACCEPTOR> {
public:

  TcpAcceptor(TcpTransport* transport_impl);
  virtual ~TcpAcceptor();

  // Returns a reference that the caller becomes responsible for.
  TcpTransport* transport();

  // This causes the Acceptor to drop its refernce to the
  // TcpTransport object.
  void transport_shutdown();

  TcpInst* get_configuration();

private:

  TcpTransport_rch transport_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_TCPACCEPTOR_H */
