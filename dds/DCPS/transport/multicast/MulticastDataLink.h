/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/transport/framework/DataLink.h"

#include "Multicast_Export.h"

#ifndef DCPS_MULTICASTDATALINK_H
#define DCPS_MULTICASTDATALINK_H

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export MulticastDataLink
  : public DataLink {
public:
  virtual ~MulticastDataLink();

protected:
  virtual void stop_i();
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTDATALINK_H */
