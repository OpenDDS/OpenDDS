// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTIMPL_H
#define TAO_DCPS_TRANSPORTIMPL_H

#include  "dds/DCPS/dcps_export.h"
#include  "dds/DCPS/RcObject_T.h"
#include  "dds/DdsDcpsInfoUtilsC.h"
#include  "TransportDefs.h"
#include  "TransportConfiguration_rch.h"
#include  "TransportReactorTask_rch.h"
#include  "ace/Hash_Map_Manager.h"
#include  "ace/Synch.h"


namespace TAO
{
  namespace DCPS
  {

    class TransportInterface;
    class TransportReceiveListener;
    class ThreadSynchStrategy;
    class TransportImplFactory;
    class TransportFactory;
    class DataLink;
    class DataWriterImpl;
    class DataReaderImpl;

    class TAO_DdsDcps_Export TransportImpl : public RcObject<ACE_SYNCH_MUTEX>
    {
      public:

        virtual ~TransportImpl();

        /// Called by the client application some time soon after this
        /// TransportImpl has been created.  This *must* be called
        /// prior to attempting to attach this TransportImpl to a
        /// TransportInterface object.
        int configure(TransportConfiguration* config);


        /// Called by the PublisherImpl to register datawriter upon
        /// datawriter creation.
        int register_publication (RepoId pub_id, DataWriterImpl* dw);
        
        /// Called by the PublisherImpl to unregister datawriter upon
        /// datawriter destruction.
        int unregister_publication (RepoId pub_id);
        
        /// Called by the DataLink to find the registered datawriter
        /// for the lost publication notification.
        DataWriterImpl* find_publication (RepoId pub_id);

        /// Called by the SubscriberImpl to register datareader upon
        /// datareader creation.
        int register_subscription (RepoId sub_id, DataReaderImpl* dr);
        
        /// Called by the SubscriberImpl to unregister datareader upon
        /// datareader destruction.
        int unregister_subscription (RepoId sub_id);
        
        /// Called by the DataLink to find the registered datareader
        /// for the lost subscription notification.
        DataReaderImpl* find_subscription (RepoId sub_id);

      protected:

        TransportImpl();

        /// If connect_as_publisher == 1, then this find_or_create_datalink()
        /// call is being made on behalf of a local publisher id association
        /// with a remote subscriber id.  If connect_as_publisher == 0, then
        /// this find_or_create_datalink() call is being made on behalf of a
        /// local subscriber id association with a remote publisher id.
        /// Note that this "flag" is only used if the find operation fails,
        /// and a new DataLink must created and go through connection
        /// establishment.  This allows the connection establishment logic
        /// to determine whether an active or passive connection needs to
        /// be made.  If the find operation works, then we don't need to
        /// establish a connection since the existing DataLink is already
        /// connected.
        virtual DataLink* find_or_create_datalink
                    (const TransportInterfaceInfo& remote_info,
                     int                           connect_as_publisher) = 0;

        /// Concrete subclass gets a shot at the config object.  The subclass
        /// will likely downcast the TransportConfiguration object to a
        /// subclass type that it expects/requires.
        virtual int configure_i(TransportConfiguration* config) = 0;

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
        virtual int connection_info_i
                              (TransportInterfaceInfo& local_info) const = 0;

        /// Called by our release_datalink() method in order to give the
        /// concrete TransportImpl subclass a chance to do something when
        /// the release_datalink "event" occurs.
        virtual void release_datalink_i(DataLink* link) = 0;

        /// Accessor to obtain a "copy" of the reference to the reactor task.
        /// Caller is responsible for the "copy" of the reference that is
        /// returned.
        TransportReactorTask* reactor_task();


      private:

        /// We have a few friends in the transport framework so that they
        /// can access our private methods.  We do this to avoid pollution
        /// of our public interface with internal framework methods.
        friend class TransportInterface;
        friend class TransportImplFactory;
        friend class TransportFactory;
        friend class DataLink;
//MJM: blech.

        /// Our friend (and creator), the TransportImplFactory, will invoke
        /// this method in order to provide us with the TransportReactorTask.
        /// This happens immediately following the creation of this
        /// TransportImpl object.
        int set_reactor(TransportReactorTask* task);

        /// Called by the TransportFactory when this TransportImpl object
        /// is released while the TransportFactory is handling a release()
        /// "event".
        void shutdown();

        /// The DataLink itself calls this method when it thinks it is
        /// no longer used for any associations.  This occurs during
        /// a "remove associations" operation being performed by some
        /// TransportInterface that uses this TransportImpl.  The
        /// TransportInterface is known to have acquired our reservation_lock_,
        /// so there won't be any reserve_datalink() calls being made from
        /// any other threads while we perform this release.
        void release_datalink(DataLink* link);

        /// Called by our friend, the TransportInterface, to attach
        /// itself to this TransportImpl object.
        AttachStatus attach_interface(TransportInterface* interface);

        /// Called by our friend, the TransportInterface, to detach
        /// itself to this TransportImpl object.
        void detach_interface(TransportInterface* interface);

        /// Called by our friend, the TransportInterface, to reserve
        /// a DataLink for a remote subscription association
        /// (a local "publisher" to a remote "subscriber" association).
        DataLink* reserve_datalink
                      (const TransportInterfaceInfo& remote_subscriber_info,
                       RepoId                        subscriber_id,
                       RepoId                        publisher_id,
                       CORBA::Long                   priority);

        /// Called by our friend, the TransportInterface, to reserve
        /// a DataLink for a remote publication association
        /// (a local "subscriber" to a remote "publisher" association).
        DataLink* reserve_datalink
                      (const TransportInterfaceInfo& remote_publisher_info,
                       RepoId                        publisher_id,
                       RepoId                        subscriber_id,
                       TransportReceiveListener*     receive_listener,
                       CORBA::Long                   priority);
//MJM: You might be able to collapse these two methods by just providing
//MJM: a default NULL recieve listener (as the last arg) as long as this
//MJM: collapsing propogates down to the datalink as well.  The method
//MJM: implementations are remarkably similar.
protected:
        typedef ACE_SYNCH_MUTEX                ReservationLockType;
        typedef ACE_Guard<ReservationLockType> ReservationGuardType;
        /// Called by our friends, the TransportInterface, and the DataLink.
        /// Since this TransportImpl can be attached to many TransportInterface
        /// objects, and each TransportInterface object could be "running" in
        /// a separate thread, we need to protect all of the "reservation"
        /// methods with a lock.  The protocol is that a client of ours
        /// must "acquire" our reservation_lock() before it can proceed to
        /// call any methods that affect the DataLink reservations.  It
        /// should release the reservation_lock() as soon as it is done.
        ReservationLockType& reservation_lock();
        const ReservationLockType& reservation_lock() const;
private:
        /// Called by our friend, the TransportInterface.
        /// Accessor for the TransportInterfaceInfo.  Accepts a reference
        /// to a TransportInterfaceInfo object that will be "populated"
        /// with this TransportImpl's connection information (ie, how
        /// another process would connect to this TransportImpl).
        int connection_info(TransportInterfaceInfo& local_info) const;

        /// Called by our friend, the TransportInterface.
        /// Accessor for the "swap bytes" flag that was supplied to this
        /// TransportImpl via the TransportConfiguration object supplied
        /// to our configure() method.
        int swap_bytes() const;


        typedef ACE_Hash_Map_Manager_Ex<void*,
                                        TransportInterface*,
                                        ACE_Hash<void*>,
                                        ACE_Equal_To<void*>,
                                        ACE_Null_Mutex>      InterfaceMapType;

        typedef ACE_SYNCH_MUTEX     LockType;
        typedef ACE_Guard<LockType> GuardType;

        typedef ACE_Hash_Map_Manager_Ex
                               <RepoId,
                                DataWriterImpl*,
                                ACE_Hash<RepoId>,
                                ACE_Equal_To<RepoId>,
                                ACE_Null_Mutex>              PublicationObjectMap;

        typedef ACE_Hash_Map_Manager_Ex
                               <RepoId,
                                DataReaderImpl*,
                                ACE_Hash<RepoId>,
                                ACE_Equal_To<RepoId>,
                                ACE_Null_Mutex>              SubscriptionObjectMap;


        /// The collection of the DataWriterImpl objects that are created by
        /// the PublisherImpl currently "attached" to this TransportImpl.
        PublicationObjectMap  dw_map_;
        
        /// The collection of the DataWriterImpl objects that are created by
        /// the SubscriberImpl currently "attached" to this TransportImpl.
        SubscriptionObjectMap dr_map_;
        /// Our reservation lock.
        ReservationLockType reservation_lock_;

        /// Lock to protect the interfaces_, config_ and reactor_task_
        /// data members.
        mutable LockType lock_;

        /// The collection of TransportInterface objects that are currently
        /// "attached" to this TransportImpl.
        InterfaceMapType interfaces_;

        /// A reference (via a smart pointer) to the TransportConfiguration
        /// object that was supplied to us during our configure() method.
        TransportConfiguration_rch config_;
//MJM: I still don't understand why this is shareable.

        /// The reactor (task) object - may not even be used if the concrete
        /// subclass (of TransportImpl) doesn't require a reactor.
        TransportReactorTask_rch reactor_task_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "TransportImpl.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_TRANSPORTIMPL_H */
