// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTINTERFACE_H
#define TAO_DCPS_TRANSPORTINTERFACE_H

#include  "dds/DCPS/dcps_export.h"
#include  "TransportImpl_rch.h"
#include  "DataLinkSetMap.h"
#include  "TransportDefs.h"
#include  "dds/DCPS/Definitions.h"
#include  "ace/Synch.h"

class ACE_Message_Block;


namespace TAO
{
  namespace DCPS
  {

    class  TransportSendListener;
    class  TransportReceiveListener;
    struct AssociationData;

    class  DataSampleList;
    struct DataSampleListElement;


    class TAO_DdsDcps_Export TransportInterface
    {
      public:

        virtual ~TransportInterface();

        /// Accessor for the TransportInterfaceInfo.
        const TransportInterfaceInfo& connection_info() const;

        /// Accessor for the swap_bytes configuration value.
        int swap_bytes() const;

        /// These methods are made public for the need of datawriter,
        /// but they are just for internal use. 

        /// Send the control message to remote subscribers through the
        /// appropriate DataLink(s).
        ///
        /// The (local) publisher_id dictates which DataLinks(s) (if any)
        /// will be used to send the control message.  This ultimately gets
        /// it to all remote subscribers interested in the local publisher_id.
        SendControlStatus send_control(RepoId                 pub_id,
                                       TransportSendListener* listener,
                                       ACE_Message_Block*     msg);

        /// Called to remove the sample from any DataLink queues that it
        /// may be stuck in.  This basically means that the caller wants
        /// his loan back.  We always acknowledge that we are returning
        /// a loaned sample by calling either data_delivered() or
        /// data_dropped() on the TransportSendListener pointed to by
        /// the sample object.  If this remove_sample() operation
        /// actually does cause the sample to be removed from some
        /// DataLink queue(s), then the callback will be the data_dropped()
        /// variety.  If a sample doesn't exist in any queues, then the
        /// data_delivered() has already been called, or is in the process
        /// of being called when this method was invoked.  The point is
        /// that the TransportSendListener will receive exactly one of
        /// the callbacks per sample sent.
        /// Returns 0 for success (whether or not the sample was
        /// removed).  Returns -1 if some fatal error was encountered
        /// along the way.
        int remove_sample(const DataSampleListElement* sample, bool dropped_by_transport);

        int remove_all_control_msgs(RepoId pub_id);

        /// This method is called by the client application (or a subclass
        /// of TransportInterface) in order to attach this TransportInterface
        /// to the supplied TransportImpl object.
        AttachStatus attach_transport(TransportImpl* impl);

        /// Return the TransportImpl object reference.
        TransportImpl_rch get_transport_impl ();

      protected:

        TransportInterface();

        /// A Publisher will invoke this method to add associations to
        /// the transport interface.
        /// Returns 0 if successful, -1 if unsuccessful
        int add_subscriptions(RepoId                 publisher_id,
                              CORBA::Long            priority,
                              ssize_t                size,
                              const AssociationData* subscriptions);

        /// A Subscriber will invoke this method to add associations to
        /// the transport interface.
        /// Returns 0 if successful, -1 if unsuccessful
        int add_publications(RepoId                    subscriber_id,
                             TransportReceiveListener* receive_listener,
                             CORBA::Long               priority,
                             ssize_t                   size,
                             const AssociationData*    publications);

        /// Subscribers will supply an array of publisher ids,
        /// and view this as a call to "remove publications".
        ///
        /// Conversely, Publishers will supply an array of subscriber ids,
        /// and view this as a call to "remove subscriptions".
        void remove_associations(ssize_t size, const RepoId* remote_ids);

        /// Send samples to remote subscribers through the appropriate
        /// DataLink(s).
        ///
        /// Each sample in the DataSampleList is from a particular
        /// local publisher_id that dictates which DataLink(s) (if any)
        /// will be used to send the sample.  This ultimately gets it
        /// to all remote subscribers interested in the local publisher_id.
        void send(const DataSampleList& samples);

        /// This method can be called by our subclass to detach this
        /// TransportInterface from the TransportImpl to which it is
        /// currently attached.  This is the opposite of the
        /// "attach_transport()" method.
        void detach_transport();

        /// Method to be implemented by subclass so that it may be
        /// informed of a transport_detached() "event".  The subclass
        /// should implement this such that it stops calling methods on
        /// this TransportInterface object, and then returns to allow
        /// the detachment to complete.
        ///
        /// Default implementation does nothing.
        virtual void transport_detached_i();


      private:

        /// We trust the TransportImpl as our friend.  It needs to call
        /// our private attach_transport() and detach_transport() methods.
        friend class TransportImpl;
//MJM: yuk.

        /// Called by the TransportImpl when the TransportFactory has
        /// told the TransportImpl to shutdown.  The TransportImpl will
        /// call this transport_detached() method on each TransportInterface
        /// still associated with the TransportImpl.
        void transport_detached();

        /// Generic algorithm used by add_publications and add_subscriptions,
        /// with the arguments determining the context (ie, local vs. remote
        /// and subscriber vs. publisher).
        int add_associations
                       (RepoId                    local_id,
                        CORBA::Long               priority,
                        const char*               local_id_str,
                        const char*               remote_id_str,
                        size_t                    num_remote_associations,
                        const AssociationData*    remote_associations,
                        TransportReceiveListener* receive_listener = 0);


        typedef ACE_SYNCH_MUTEX     LockType;
        typedef ACE_Guard<LockType> GuardType;

        /// Used to protect the impl_ data member, and our state_.
        mutable LockType lock_;

        /// The TransportImpl object that represents our local endpoint
        /// for each DataLink object it provides to us.
        TransportImpl_rch impl_;

        /// Note: Not protected by any lock.
        /// The (cached) connection information (blob) obtained  
        /// from the TransportImpl during attach_transport().
        TransportInterfaceInfo connection_info_;

        /// Note: Not protected by any lock.
        /// The (cached) swap bytes configuration value obtained  
        /// from the TransportImpl during attach_transport().
        int swap_bytes_;

        /// Note: Not protected by any lock.
        /// Map used to associate a local_id with the set of DataLinks that
        /// contain at least one "reservation" for the local_id.
        DataLinkSetMap local_map_;

        /// Note: Not protected by any lock.
        /// Map used to associate a remote_id with the set of DataLinks that
        /// contain at least one "reservation" for the remote_id.
        DataLinkSetMap remote_map_;

        /// Note: Not protected by any lock.
        /// A set of DataLinks used by the send() method to collect up the
        /// unique set of DataLinks that it has sent something to.  This
        /// set will then be used to tell each unique DataLink that this
        /// round of send() calls is done.  This was made a data member
        /// to avoid the creation of the set each time.
        DataLinkSet_rch send_links_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "TransportInterface.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_TRANSPORTINTERFACE_H */
