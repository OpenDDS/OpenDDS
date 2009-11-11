/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/transport/framework/TransportGenerator.h"

#include "Multicast_Export.h"

#ifndef DCPS_MULTICASTGENERATOR_H
#define DCPS_MULTICASTGENERATOR_H

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastGenerator
  : public TransportGenerator {
public:
  virtual ~MulticastGenerator();

  virtual TransportImplFactory* new_factory();

  virtual TransportConfiguration* new_configuration(const TransportIdType id);

  virtual void default_transport_ids(TransportIdList& ids);
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTGENERATOR_H */
