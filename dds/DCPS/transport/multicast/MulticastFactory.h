/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTFACTORY_H
#define DCPS_MULTICASTFACTORY_H

#include "Multicast_Export.h"

#include "dds/DCPS/transport/framework/TransportImplFactory.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastFactory
  : public TransportImplFactory {
public:
  virtual int requires_reactor() const;

protected:
  virtual TransportImpl* create();
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTFACTORY_H */
