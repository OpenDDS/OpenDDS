/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_MULTICASTSESSIONFACTORY_H
#define DCPS_MULTICASTSESSIONFACTORY_H

#include "Multicast_Export.h"

#include "MulticastTypes.h"

#include "ace/Synch_Traits.h"

#include "dds/DCPS/RcObject_T.h"

namespace OpenDDS {
namespace DCPS {

class MulticastDataLink;
class MulticastSession;

class OpenDDS_Multicast_Export MulticastSessionFactory
  : public RcObject<ACE_SYNCH_MUTEX> {
public:
  virtual ~MulticastSessionFactory();

  virtual int requires_send_buffer() const = 0;

  virtual MulticastSession* create(MulticastDataLink* link,
                                   MulticastPeer remote_peer) = 0;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_MULTICASTSESSIONFACTORY_H */
