/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_TCPFACTORY_H
#define OPENDDS_TCPFACTORY_H

#include "Tcp_export.h"

#include "dds/DCPS/transport/framework/TransportImplFactory.h"

class TcpTransport;

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Tcp_Export TcpFactory : public TransportImplFactory {
public:

  TcpFactory();
  virtual ~TcpFactory();

  virtual int requires_reactor() const;

protected:

  virtual TransportImpl* create();
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "TcpFactory.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DDS_SIMPLETCPFACTORY_H */
