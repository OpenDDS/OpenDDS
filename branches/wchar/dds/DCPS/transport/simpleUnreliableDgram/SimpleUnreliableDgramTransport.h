// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAMTRANSPORT_H
#define OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAMTRANSPORT_H

#include "SimpleUnreliableDgram_export.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "SimpleUnreliableDgramDataLink.h"
#include "SimpleUnreliableDgramDataLink_rch.h"
#include "SimpleUnreliableDgramSocket_rch.h"
#include "SimpleUnreliableDgramConfiguration.h"
#include "SimpleUnreliableDgramConfiguration_rch.h"
#include "dds/DCPS/transport/framework/TransportReceiveStrategy_rch.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"
#include "ace/INET_Addr.h"
#include "ace/Hash_Map_Manager.h"
#include "ace/Synch.h"


namespace OpenDDS
{

  namespace DCPS
  {

    class SimpleUnreliableDgramReceiveStrategy;
    class ReceivedDataSample;

    /**
     * This class provides specific implementation for unreliable transports - UDP and 
     * unreliable multicast.
     *
     * Notes about object ownership:
     * 1) Own datalink objects and socket object.
     * 2) Reference to TransportReactorTask object, configuration object 
     *    and TransportReceiveStrategy object.
     */
    class SimpleUnreliableDgram_Export SimpleUnreliableDgramTransport : public TransportImpl
    {
      public:

        SimpleUnreliableDgramTransport();
        virtual ~SimpleUnreliableDgramTransport();

      protected:

        friend class SimpleUnreliableDgramSynchResource;

        virtual DataLink* find_or_create_datalink
                          (const TransportInterfaceInfo& remote_info,
                           int                           connect_as_publisher);

        virtual int configure_i(TransportConfiguration* config);
 
        virtual int configure_socket(TransportConfiguration* config) = 0;
  
        virtual void shutdown_i();

        virtual int connection_info_i
                                 (TransportInterfaceInfo& local_info) const = 0;

        virtual bool acked (RepoId);

        virtual void notify_lost_on_backpressure_timeout ();

        /// Called by the DataLink to release itself.
        virtual void release_datalink_i(DataLink* link);

      protected:

        /// Our friend needs to call our deliver_sample() method.
        friend class SimpleUnreliableDgramReceiveStrategy;

        virtual void deliver_sample(ReceivedDataSample&  sample,
                            const ACE_INET_Addr& remote_address);

        /// Map Type: (key) ACE_INET_Addr to (value) SimpleMcastDataLink_rch
        typedef ACE_Hash_Map_Manager_Ex
                               <ACE_INET_Addr,
                                SimpleUnreliableDgramDataLink_rch,
                                ACE_Hash<ACE_INET_Addr>,
                                ACE_Equal_To<ACE_INET_Addr>,
                                ACE_Null_Mutex>              AddrLinkMap;

        typedef ACE_SYNCH_MUTEX         LockType;
        typedef ACE_Guard<LockType>     GuardType;
        typedef ACE_Condition<LockType> ConditionType;

        /// This is the map of connected DataLinks.
        AddrLinkMap links_;

        /// This lock is used to protect the links_ data member.
        LockType links_lock_;

        /// We need the reactor to tell our socket to handle_input().
        TransportReactorTask_rch reactor_task_;

        /// The TransportReceiveStrategy object for this TransportImpl.
        TransportReceiveStrategy_rch receive_strategy_;

        SimpleUnreliableDgramSocket_rch  socket_;

        SimpleUnreliableDgramConfiguration_rch config_;
    };

  } /* namespace DCPS */

} /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "SimpleUnreliableDgramTransport.inl"
#endif /* __ACE_INLINE__ */


#endif  /* OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAMTRANSPORT_H */
