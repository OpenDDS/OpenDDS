/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_MULTICAST_MULTICASTSESSIONFACTORY_H
#define OPENDDS_DCPS_TRANSPORT_MULTICAST_MULTICASTSESSIONFACTORY_H

#include "MulticastTypes.h"
#include "Multicast_Export.h"

#include "dds/DCPS/RcHandle_T.h"
#include "dds/DCPS/RcObject.h"
#include "dds/DCPS/ReactorTask.h"

#include "ace/Synch_Traits.h"

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Reactor;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class MulticastDataLink;
class MulticastSession;
typedef RcHandle<MulticastSession> MulticastSession_rch;

class OpenDDS_Multicast_Export MulticastSessionFactory
  : public virtual RcObject {
public:
  virtual ~MulticastSessionFactory();

  virtual int requires_send_buffer() const = 0;

  virtual MulticastSession_rch create(RcHandle<ReactorTask> reactor_task,
                                      MulticastDataLink* link,
                                      MulticastPeer remote_peer) = 0;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_MULTICASTSESSIONFACTORY_H */
