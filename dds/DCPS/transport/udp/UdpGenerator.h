/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_UDPGENERATOR_H
#define DCPS_UDPGENERATOR_H

#include "Udp_Export.h"

#include "dds/DCPS/transport/framework/TransportGenerator.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Udp_Export UdpGenerator
  : public TransportGenerator {
public:
  virtual TransportImplFactory* new_factory();

  virtual TransportConfiguration* new_configuration(const TransportIdType id);

  virtual void default_transport_ids(TransportIdList& ids);
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_UDPGENERATOR_H */
