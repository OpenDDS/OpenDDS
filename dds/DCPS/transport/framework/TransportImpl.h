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
#include "PriorityKey.h"
#include "ace/Synch.h"
#include <map>
#include <set>
#include <string>

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

  /// Callback from the DataLink to clean up any associated resources.
  /// This usually is done when the DataLink is lost. The call is made with
  /// no transport/DCPS locks held.
  bool release_link_resources(DataLink* link);

  /// Expose the configuration information so others can see what
  /// we can do.
  TransportInst* config() const;

  /// Called by our connection_info() method to allow the concrete
  /// TransportImpl subclass to do the dirty work since it really
  /// is the one that knows how to populate the supplied
  /// TransportLocator object.
  virtual bool connection_info_i(TransportLocator& local_info) const = 0;

  /// Diagnostic aid.
  void dump();
  void dump(ostream& os);

  void report();

protected:
  TransportImpl();

  bool configure(TransportInst* config);

  struct ConnectionAttribs {
    CORBA::Long priority_;
    bool local_reliable_, local_durable_;
  };

  DataLink* find_datalink(const RepoId& local_id,
                          const AssociationData& remote_association,
                          const ConnectionAttribs& attribs,
                          bool active);

  DataLink* connect_datalink(const RepoId& local_id,
                             const AssociationData& remote_association,
                             const ConnectionAttribs& attribs);

  struct OpenDDS_Dcps_Export ConnectionEvent {
    ConnectionEvent(const RepoId& local_id,
                    const AssociationData& remote_association,
                    const ConnectionAttribs& attribs);

    void wait(const ACE_Time_Value& timeout);
    bool complete(const DataLink_rch& link);

    const RepoId& local_id_;
    const AssociationData& remote_association_;
    const ConnectionAttribs& attribs_;
    ACE_Thread_Mutex mtx_;
    ACE_Condition_Thread_Mutex cond_;
    DataLink_rch link_;
  };

  /// accept_datalink() is called from TransportClient::associate()
  /// for each TransportImpl on the passive side.  Any datalink
  /// returned should be a fully connected datalink matching this
  /// connection event.  Otherwise, this operation should return
  /// 0 and passively wait for physical connection from the active
  /// side (either in the form of a connection event or handshaking
  /// message).  Upon completion of the physical connection, the
  /// transport should signal the connection event, as
  /// TransportClient::associate() is waiting on this.
  virtual DataLink* accept_datalink(ConnectionEvent& ce) = 0;

  /// connect_datalink_i() is called from TransportClient::associate()
  /// (via TransportImpl::connect_datalink()) for each TransportImpl
  /// on the active side, until a valid datalink is returned.  Any
  /// datalink returned should be a fully connected datalink matching
  /// this connection event.  This method should block and wait for
  /// the physical connection (either in the form of a connection
  /// event or handshaking message).  If the connection is unable to
  /// be completed, returning 0 causes TransportClient::associate()
  /// to try the next TransportImpl in the current configuration.
  virtual DataLink* connect_datalink_i(const RepoId& local_id,
                                       const RepoId& remote_id,
                                       const TransportBLOB& remote_data,
                                       bool remote_reliable,
                                       const ConnectionAttribs& attribs) = 0;

  /// stop_accepting() is called from TransportClient::associate()
  /// to terminate the accepting process begun by accept_datalink().
  /// This allows the TransportImpl to clean up any resources
  /// asssociated with this ConnectionEvent.
  virtual void stop_accepting(ConnectionEvent& ce) = 0;

  /// find_datalink_i() attempts to return a DataLink that was
  /// previously created via either accept_datalink() or
  /// connect_datalink_i().  If one is not available,
  /// the transport framework proceeds with creating a new
  /// datalink.
  virtual DataLink* find_datalink_i(const RepoId& local_id,
                                    const RepoId& remote_id,
                                    const TransportBLOB& remote_data,
                                    bool remote_reliable,
                                    const ConnectionAttribs& attribs,
                                    bool active) = 0;


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
  /// TransportClient is known to have acquired our reservation_lock_,
  /// so there won't be any reserve_datalink() calls being made from
  /// any other threads while we perform this release.
  virtual void release_datalink(DataLink* link) = 0;

  void attach_client(TransportClient* client);
  void detach_client(TransportClient* client);
  virtual void pre_detach(TransportClient*) {}

  DataLink* find_connect_i(const RepoId& local_id,
                           const AssociationData& remote_association,
                           const ConnectionAttribs& attribs,
                           bool active, bool connect);

public:
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

  /// Create the reactor task using sync send or optionally async send
  /// by parameter on supported Windows platforms only.
  void create_reactor_task(bool useAsyncSend = false);

private:
  /// Called by our friend, the TransportClient.
  /// Accessor for the TransportInterfaceInfo.  Accepts a reference
  /// to a TransportInterfaceInfo object that will be "populated"
  /// with this TransportImpl's connection information (ie, how
  /// another process would connect to this TransportImpl).
  bool connection_info(TransportLocator& local_info) const;

  virtual std::string transport_type() const = 0;

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
