/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_UDPFACTORY_H
#define DCPS_UDPFACTORY_H

#include "Udp_Export.h"

#include "dds/DCPS/transport/framework/TransportImplFactory.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Udp_Export UdpFactory
  : public TransportImplFactory {
public:
  virtual int requires_reactor() const;

protected:
  virtual TransportImpl* create();
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_UDPFACTORY_H */
