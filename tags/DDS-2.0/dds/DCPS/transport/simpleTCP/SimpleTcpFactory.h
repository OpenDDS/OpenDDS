/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SIMPLETCPFACTORY_H
#define OPENDDS_DCPS_SIMPLETCPFACTORY_H

#include "SimpleTcp_export.h"

#include "dds/DCPS/transport/framework/TransportImplFactory.h"

class SimpleTcpTransport;

namespace OpenDDS {
namespace DCPS {

class SimpleTcp_Export SimpleTcpFactory : public TransportImplFactory {
public:

  SimpleTcpFactory();
  virtual ~SimpleTcpFactory();

  virtual int requires_reactor() const;

protected:

  virtual TransportImpl* create();
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "SimpleTcpFactory.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DDS_SIMPLETCPFACTORY_H */
