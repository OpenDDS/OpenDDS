// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEMCASTTRANSPORT_H
#define TAO_DCPS_SIMPLEMCASTTRANSPORT_H

#include  "SimpleMcast_export.h"
#include  "dds/DCPS/transport/framework/TransportImpl.h"
//borland #include  "SimpleMcastConfiguration.h"
#include  "SimpleMcastConfiguration_rch.h"
//borland #include  "SimpleMcastDataLink.h"
#include  "SimpleMcastDataLink_rch.h"
//borland #include  "SimpleMcastSocket.h"
#include  "SimpleMcastSocket_rch.h"
#include  "dds/DCPS/transport/framework/TransportReceiveStrategy_rch.h"
//borland #include  "dds/DCPS/transport/framework/TransportReactorTask.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask_rch.h"
#include  "ace/INET_Addr.h"
#include  "ace/Hash_Map_Manager.h"
#include  "ace/Synch.h"


namespace TAO
{

  namespace DCPS
  {

    class SimpleMcastReceiveStrategy;
    class ReceivedDataSample;


    class SimpleMcast_Export SimpleMcastTransport : public TransportImpl
    {
      public:

        SimpleMcastTransport();
        virtual ~SimpleMcastTransport();

        virtual bool acked (RepoId pub_id);

        void notify_lost_on_backpressure_timeout ();


      protected:

        virtual DataLink* find_or_create_datalink
                          (const TransportInterfaceInfo& remote_info,
                           int                           connect_as_publisher);

        virtual int configure_i(TransportConfiguration* config);

        virtual void shutdown_i();

        virtual int connection_info_i
                                 (TransportInterfaceInfo& local_info) const;

        /// Called by the DataLink to release itself.
        virtual void release_datalink_i(DataLink* link);

      private:

        /// Our friend needs to call our deliver_sample() method.
        friend class SimpleMcastReceiveStrategy;

        void deliver_sample(ReceivedDataSample&  sample,
                            const ACE_INET_Addr& remote_address);

        /// Map Type: (key) ACE_INET_Addr to (value) SimpleMcastDataLink_rch
        typedef ACE_Hash_Map_Manager_Ex
                               <ACE_INET_Addr,
                                SimpleMcastDataLink_rch,
                                ACE_Hash<ACE_INET_Addr>,
                                ACE_Equal_To<ACE_INET_Addr>,
                                ACE_Null_Mutex>              AddrLinkMap;

        typedef ACE_SYNCH_MUTEX         LockType;
        typedef ACE_Guard<LockType>     GuardType;
        typedef ACE_Condition<LockType> ConditionType;

        /// Our configuration object, supplied to us in config_i().
        SimpleMcastConfiguration_rch mcast_config_;

        /// This is the map of connected DataLinks.
        AddrLinkMap links_;

        /// This lock is used to protect the links_ data member.
        LockType links_lock_;

        /// We need the reactor to tell our socket to handle_input().
        TransportReactorTask_rch reactor_task_;

        /// The TransportReceiveStrategy object for this TransportImpl.
        TransportReceiveStrategy_rch receive_strategy_;

        /// There is just one SimpleMcastSocket object per SimpleMcastTransport
        /// object, and is shared amongst all SimpleMcastDataLink objects
        /// created by this SimpleMcastTransport object.
        SimpleMcastSocket_rch socket_;

        ACE_INET_Addr local_address_;
        ACE_INET_Addr multicast_group_address_;
        bool receiver_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleMcastTransport.inl"
#endif /* __ACE_INLINE__ */


#endif  /* TAO_DCPS_SIMPLEMCASTTRANSPORT_H */
