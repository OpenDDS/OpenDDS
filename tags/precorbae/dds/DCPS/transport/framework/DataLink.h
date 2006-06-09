// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_DATALINK_H
#define TAO_DCPS_DATALINK_H

#include  "dds/DCPS/dcps_export.h"
#include  "dds/DCPS/RcObject_T.h"
#include  "TransportDefs.h"
#include  "ReceiveListenerSetMap.h"
#include  "RepoIdSetMap.h"
#include  "TransportImpl_rch.h"
#include  "TransportSendStrategy.h"
#include  "TransportSendStrategy_rch.h"
#include  "TransportReceiveStrategy.h"
#include  "TransportReceiveStrategy_rch.h"
#include  "ace/Synch.h"


namespace TAO
{

  namespace DCPS
  {

    class  TransportReceiveListener;
    class  TransportQueueElement;
    class  DataLinkSetMap;
    class  ReceivedDataSample;
    struct DataSampleListElement;


    class TAO_DdsDcps_Export DataLink : public RcObject<ACE_SYNCH_MUTEX>
    {
      public:

        enum ConnectionNotice{
          DISCONNECTED,
          RECONNECTED,
          LOST
        };

        /// A DataLink object is always created by a TransportImpl object.
        /// Thus, the TransportImpl object passed-in here is the object that
        /// created this DataLink.
        DataLink(TransportImpl* impl);
        virtual ~DataLink();

        /// This is for a remote subscriber_id and local publisher_id
        int make_reservation(RepoId subscriber_id, RepoId publisher_id);

        /// This is for a remote publisher_id and a local subscriber_id.
        /// The TransportReceiveListener is associated with the local
        /// subscriber_id.
        int make_reservation(RepoId                    publisher_id,
                             RepoId                    subscriber_id,
                             TransportReceiveListener* receive_listener);

        /// This will release reservations that were made by one of the
        /// make_reservation() methods.  All we know is that the supplied
        /// RepoId is considered to be a remote id.  It could be a
        /// remote subscriber or a remote publisher.
        void release_reservations(RepoId          remote_id,
                                  DataLinkSetMap& released_locals);

        /// Called by the TransportInterface objects that reference this
        /// DataLink.  Used by the TransportInterface to send a sample,
        /// or to send a control message.
        void send_start();
        void send(TransportQueueElement* element);
        void send_stop();
//MJM: We may want to change these to be enable/disable instead of start/stop.
//MJM: No real reason, but the semantics may be easier to understand?

        /// This method is essentially an "undo_send()" method.  It's goal
        /// is to remove all traces of the sample from this DataLink (if
        /// the sample is even known to the DataLink).
        /// A return value of -1 indicates that a fatal error was encountered
        /// while trying to carry out the remove_sample operation.
        /// A return value of 0 indicates that there was no fatal error, and
        /// that this DataLink no longer references the sample (if it ever
        /// did).
        int remove_sample(const DataSampleListElement* sample, bool dropped_by_transport);

        void remove_all_control_msgs(RepoId pub_id);

        /// This is called by our TransportReceiveStrategy object when it
        /// has received a complete data sample.  This method will cause
        /// the appropriate TransportReceiveListener objects to be told
        /// that data_received().
        int data_received(ReceivedDataSample& sample);

        /// Obtain a unique identifier for this DataLink object.
        DataLinkIdType id() const;

        /// Our TransportImpl will inform us if it is being shutdown()
        /// by calling this method.
        void transport_shutdown();

        /// Notify the datawriters and datareaders that the connection is
        /// disconnected, lost, or reconnected. The datareader/datawriter
        /// will notify the corresponding listener.
        void notify (enum ConnectionNotice notice);

        /// Called before release the datalink or before shutdown to let
        /// the concrete DataLink to do anything necessary.
        virtual void pre_stop_i () {};

       protected:

        /// This is how the subclass "announces" to this DataLink base class
        /// that this DataLink has now been "connected" and should start
        /// the supplied strategy objects.  This start method is also
        /// going to keep a "copy" of the references to the strategy objects.
        /// Also note that it is acceptable to pass-in a NULL (0)
        /// TransportReceiveStrategy*, but it is assumed that the
        /// TransportSendStrategy* argument is not NULL.
        ///
        /// If the start() method fails to start either strategy, then a -1
        /// is returned.  Otherwise, a 0 is returned.  In the failure case,
        /// if one of the strategy objects was started successfully, then
        /// it will be stopped before the start() method returns -1.
        int start(TransportSendStrategy*    send_strategy,
                  TransportReceiveStrategy* receive_strategy);

        /// This announces the "stop" event to our subclass.  The "stop"
        /// event will occur when this DataLink is handling a
        /// release_reservations() call and determines that it has just
        /// released all of the remaining reservations on this DataLink.
        /// The "stop" event will also occur when the TransportImpl
        /// is being shutdown() - we call stop_i() from our
        /// transport_shutdown() method to handle this case.
        virtual void stop_i() = 0;

        /// Used to provide unique Ids to all DataLink methods.
        static ACE_UINT64 get_next_datalink_id ();

        /// The transport receive strategy object for this DataLink.
        TransportReceiveStrategy_rch receive_strategy_;

      private:

        /// Helper function to output the enum as a string to help debugging.
        const char* connection_notice_as_str (enum ConnectionNotice notice);

        /// Used by release_reservations() once it has determined that the
        /// remote_id being released is, in fact, a remote subscriber id.
        void release_remote_subscriber(RepoId          subscriber_id,
                                       RepoIdSet*      pubid_set,
                                       DataLinkSetMap& released_publishers);

        /// Used by release_reservations() once it has determined that the
        /// remote_id being released is, in fact, a remote publisher id.
        void release_remote_publisher
                                 (RepoId              publisher_id,
                                  ReceiveListenerSet* listener_set,
                                  DataLinkSetMap&     released_subscribers);

        typedef ACE_SYNCH_MUTEX     LockType;
        typedef ACE_Guard<LockType> GuardType;


        /// The lock_ protects the pub_map_ and sub_map_ data members.
        /// We need this becuase this DataLink object could be called from
        /// more than one TransportInterface (and each could be driven by
        /// a different thread).
        LockType lock_;

        /// Map associating each publisher_id with a set of
        /// TransportReceiveListener objects (each with an associated
        /// subscriber_id).  This map is used for delivery of incoming
        /// (aka, received) data samples.
        ReceiveListenerSetMap pub_map_;

        /// Map associating each subscriber_id with the set of publisher_ids.
        /// In essence, the pub_map_ and sub_map_ are the "mirror image" of
        /// each other, where we have a many (publishers) to many (subscribers)
        /// association being managed here.
        RepoIdSetMap sub_map_;

        /// A (smart) pointer to the TransportImpl that created this DataLink.
        TransportImpl_rch impl_;

        /// The transport send strategy object for this DataLink.
        TransportSendStrategy_rch send_strategy_;

        /// The id for this DataLink
        ACE_UINT64 id_;
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "DataLink.inl"
#endif /* __ACE_INLINE__ */

#endif /* TAO_DCPS_DATALINK_H */
