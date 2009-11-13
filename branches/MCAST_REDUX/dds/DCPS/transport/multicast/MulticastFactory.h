/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/transport/framework/TransportImplFactory.h"

#include "Multicast_Export.h"

#ifndef DCPS_MULTICASTFACTORY_H
#define DCPS_MULTICASTFACTORY_H

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
