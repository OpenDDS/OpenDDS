/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/RcObject_T.h"
#include "dds/DCPS/transport/framework/DataLink.h"

#include "Multicast_Export.h"

#ifndef DCPS_MULTICASTDATALINK_H
#define DCPS_MULTICASTDATALINK_H

namespace OpenDDS {
namespace DCPS {

class MulticastTransport;
class MulticastConfiguration;

class OpenDDS_Multicast_Export MulticastDataLink
  : public DataLink {
public:
  MulticastDataLink(MulticastTransport* impl,
                    long local_id,
                    CORBA::Long priority,
                    bool active);

  bool open(long remote_id);
  void close();

protected:
  virtual void stop_i();

private:
  long local_id_;
  bool active_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTDATALINK_H */
