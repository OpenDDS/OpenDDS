/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTIMPL_H
#define OPENDDS_DCPS_TRANSPORTIMPL_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject_T.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsPublicationC.h"
#include "TransportDefs.h"
#include "TransportInst.h"
#include "TransportInst_rch.h"
#include "TransportReactorTask.h"
#include "TransportReactorTask_rch.h"
#include "RepoIdSetMap.h"
#include "DataLinkCleanupTask.h"
#include "ace/Synch.h"
#include <map>
#include <set>

namespace OpenDDS {
namespace DCPS {

class TransportClient;
class TransportReceiveListener;
class ThreadSynchStrategy;
class DataLink;
class Monitor;
struct AssociationData;

/** The TransportImpl class includes the abstract methods that must be implemented
*   by any implementation to provide data delivery service to the DCPS implementation.
*   This includes methods to send data, received data, configure the operation, and
*   manage associations and datalinks between local and remote objects of the implementation.
*
*   Notes about object ownership:
*   1)Has longer lifetime than the publisher and subscriber objects. The publishers
*     and subscribers are owned by the DomainParticipant and transport factory shutdown
*     is always after DomainParticipant factory shutdown.
*   2)The concrete transport object owns the datalink objects.
*   3)Own  a DataLinkCleanup object.
*   4)Reference to TransportInst object and TransportReactorTask object owned
*     by TransportRegistry.
*   5)During transport shutdown, if this object does not have ownership of an object
*     but has a references via smart pointer then the reference should be freed;
*     if this object has ownership of task objects then the tasks should be closed.
*/
class OpenDDS_Dcps_Export TransportImpl : public RcObject<ACE_SYNCH_MUTEX> {
public:

  virtual ~TransportImpl();

  /// Called when the receive strategy received the FULLY_ASSOCIATED
  /// message.
  bool demarshal_acks(ACE_Message_Block* acks, bool swap_bytes);

  /// Return true if the subscriptions to a datawriter is
  /// acknowledged, otherwise return false.
  /// In current supported transports, only SimpleTCP requires acknowledgment.
  /// Other transports do not need acknowledgment from subscriber side so these
  /// transports need override this function to always return true.
  virtual bool acked(RepoId pub_id, RepoId sub_id);

  /// Remove the pub_id-sub_id pair from ack map.
  /// In current supported transports, only SimpleTCP requires acknowledgment so
  /// it does remove the ack from ack map.
  /// Other transports has empty ack map so these transports need override
  /// this function to be noop.
  virtual void remove_ack(RepoId pub_id, RepoId sub_id);

  /// Callback from the DataLink to clean up any associated resources.
  /// This usually is done when the DataLink is lost. The call is made with
  /// no transport/DCPS locks held.
  bool release_link_resources(DataLink* link);

  /// Expose the configuration information so others can see what
  /// we can do.
  TransportInst* config() const;

  /// This method is called when the FULLY_ASSOCIATED ack of the pending
  /// associations is received. If the datawriter is registered, the
  /// datawriter will be notified, otherwise the status of the pending
  /// associations will be marked as FULLY_ASSOCIATED.
  void check_fully_association();

  /// Diagnostic aid.
  void dump();
  void dump(ostream& os);

  void report();

protected:
  TransportImpl();

  bool configure(TransportInst* config);

  virtual DataLink* find_datalink(
    RepoId                  local_id,
    const AssociationData&  remote_association,
    CORBA::Long             priority,
    bool                    active) = 0;

  virtual DataLink* create_datalink(
    RepoId                  local_id,
    const AssociationData&  remote_association,
    CORBA::Long             priority,
    bool                    active) = 0;

  /// Concrete subclass gets a shot at the config object.  The subclass
  /// will likely downcast the TransportInst object to a
  /// subclass type that it expects/requires.
  virtual bool configure_i(TransportInst* config) = 0;

  /// Called during the shutdown() method in order to give the
  /// concrete TransportImpl subclass a chance to do something when
  /// the shutdown "event" occurs.
  virtual void shutdown_i() = 0;

  /// Called before transport is shutdown to let the
  /// concrete transport to do anything necessary.
  virtual void pre_shutdown_i();

  /// Called by our connection_info() method to allow the concrete
  /// TransportImpl subclass to do the dirty work since it really
  /// is the one that knows how to populate the supplied
  /// TransportInterfaceInfo object.
  virtual bool connection_info_i(TransportLocator& local_info) const = 0;

  /// Called by our release_datalink() method in order to give the
  /// concrete TransportImpl subclass a chance to do something when
  /// the release_datalink "event" occurs.
  virtual void release_datalink_i(DataLink* link, bool release_pending) = 0;

  /// Accessor to obtain a "copy" of the reference to the reactor task.
  /// Caller is responsible for the "copy" of the reference that is
  /// returned.
  TransportReactorTask* reactor_task();

private:
  /// We have a few friends in the transport framework so that they
  /// can access our private methods.  We do this to avoid pollution
  /// of our public interface with internal framework methods.
  friend class TransportInst;
  friend class TransportClient;
  friend class DataLink;
  /// Called by the TransportRegistry when this TransportImpl object
  /// is released while the TransportRegistry is handling a release()
  /// "event".
  void shutdown();

  /// The DataLink itself calls this method when it thinks it is
  /// no longer used for any associations.  This occurs during
  /// a "remove associations" operation being performed by some
  /// TransportClient that uses this TransportImpl.  The
  /// TransportClient is known to have acquired our reservation_lock_ [TODO],
  /// so there won't be any reserve_datalink() calls being made from
  /// any other threads while we perform this release.
  /// Since there are some delay of the datalink release, the release_pending
  /// flag means whether the release happen right away or after some delay.
  void release_datalink(DataLink* link, bool release_pending);

  void attach_client(TransportClient* client);
  void detach_client(TransportClient* client);

protected:
  typedef ACE_SYNCH_MUTEX                ReservationLockType;
  typedef ACE_Guard<ReservationLockType> ReservationGuardType;
  /// Called by our friends, the TransportClient, and the DataLink.
  /// Since this TransportImpl can be attached to many TransportClient
  /// objects, and each TransportClient object could be "running" in
  /// a separate thread, we need to protect all of the "reservation"
  /// methods with a lock.  The protocol is that a client of ours
  /// must "acquire" our reservation_lock() before it can proceed to
  /// call any methods that affect the DataLink reservations.  It
  /// should release the reservation_lock() as soon as it is done.
  ReservationLockType& reservation_lock();
  const ReservationLockType& reservation_lock() const;

  void create_reactor_task();

public:
  /// Called on publisher side as the last step of the add_associations().
  /// The pending publications are cached to pending_association_sub_map_.
  /// If the transport already received the FULLY_ASSOCIATED acks from
  /// subscribers then the transport will notify the datawriter fully
  /// association and the associations will be
  /// removed from the pending associations cache.
  bool add_pending_association(const RepoId& local_id,
                               const RepoId& remote_id,
                               TransportSendListener* tsl);

private:

  void check_fully_association(const RepoId& pub_id);
  bool check_fully_association(const RepoId& pub_id, const RepoId& sub_id);

  /// Called by our friend, the TransportClient.
  /// Accessor for the TransportInterfaceInfo.  Accepts a reference
  /// to a TransportInterfaceInfo object that will be "populated"
  /// with this TransportImpl's connection information (ie, how
  /// another process would connect to this TransportImpl).
  bool connection_info(TransportLocator& local_info) const;

  typedef ACE_SYNCH_MUTEX     LockType;
  typedef ACE_Guard<LockType> GuardType;

  /// Our reservation lock.
  ReservationLockType reservation_lock_;

  /// Lock to protect the config_ and reactor_task_ data members.
  mutable LockType lock_;

  std::set<TransportClient*> clients_;

  /// A reference (via a smart pointer) to the TransportInst
  /// object that was supplied to us during our configure() method.
  TransportInst_rch config_;

  /// The reactor (task) object - may not even be used if the concrete
  /// subclass (of TransportImpl) doesn't require a reactor.
  TransportReactorTask_rch reactor_task_;

  /// These are used by the publisher side.

  typedef std::map<PublicationId, std::vector<SubscriptionId>,
                   GUID_tKeyLessThan>
    PendingAssociationsMap;

  /// pubid -> remote sub ids map.
  PendingAssociationsMap pending_association_sub_map_;

  std::map<PublicationId, TransportSendListener*, GUID_tKeyLessThan>
    association_listeners_;

  /// Fully association acknowledged map. pubid -> *subid
  RepoIdSetMap acked_sub_map_;

  /// smart ptr to the associated DL cleanup task
  DataLinkCleanupTask dl_clean_task_;

  /// Monitor object for this entity
  Monitor* monitor_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "TransportImpl.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTIMPL_H */
