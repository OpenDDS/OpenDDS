/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RELIABLESESSIONFACTORY_H
#define DCPS_RELIABLESESSIONFACTORY_H

#include "Multicast_Export.h"

#include "MulticastSessionFactory.h"

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Multicast_Export ReliableSessionFactory
  : public MulticastSessionFactory {
public:
  virtual int requires_send_buffer() const;

  virtual MulticastSession* create(MulticastDataLink* link,
                                   MulticastPeer remote_peer);
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RELIABLESESSIONFACTORY_H */
