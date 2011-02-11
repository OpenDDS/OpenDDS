/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_BESTEFFORTSESSIONFACTORY_H
#define DCPS_BESTEFFORTSESSIONFACTORY_H

#include "Multicast_Export.h"

#include "MulticastSessionFactory.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export BestEffortSessionFactory
  : public MulticastSessionFactory {
public:
  virtual int requires_send_buffer() const;

  virtual MulticastSession* create(MulticastDataLink* link,
                                   MulticastPeer remote_peer);
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_BESTEFFORTSESSIONFACTORY_H */
