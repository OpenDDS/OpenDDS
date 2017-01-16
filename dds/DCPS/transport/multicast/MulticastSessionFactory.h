/*
 *
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
#include "dds/DCPS/RcHandle_T.h"

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
  : public RcObject<ACE_SYNCH_MUTEX> {
public:
  virtual ~MulticastSessionFactory();

  virtual int requires_send_buffer() const = 0;

  virtual MulticastSession_rch create(ACE_Reactor* reactor,
                                      ACE_thread_t owner,
                                      MulticastDataLink* link,
                                      MulticastPeer remote_peer) = 0;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_MULTICASTSESSIONFACTORY_H */
