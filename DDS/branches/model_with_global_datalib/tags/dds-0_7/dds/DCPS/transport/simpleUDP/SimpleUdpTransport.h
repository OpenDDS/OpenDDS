// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUDPTRANSPORT_H
#define TAO_DCPS_SIMPLEUDPTRANSPORT_H

#include  "dds/DCPS/dcps_export.h"
#include  "dds/DCPS/transport/framework/TransportImpl.h"
#include  "SimpleUdpConfiguration_rch.h"
#include  "SimpleUdpDataLink_rch.h"
#include  "SimpleUdpSocket_rch.h"
#include  "dds/DCPS/transport/framework/TransportReceiveStrategy_rch.h"
#include  "dds/DCPS/transport/framework/TransportReactorTask_rch.h"
#include  "ace/INET_Addr.h"
#include  "ace/Hash_Map_Manager.h"
#include  "ace/Synch.h"


namespace TAO
{

  namespace DCPS
  {

    class SimpleUdpReceiveStrategy;
    class ReceivedDataSample;


    class TAO_DdsDcps_Export SimpleUdpTransport : public TransportImpl
    {
      public:

        SimpleUdpTransport();
        virtual ~SimpleUdpTransport();


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
        friend class SimpleUdpReceiveStrategy;

        void deliver_sample(ReceivedDataSample&  sample,
                            const ACE_INET_Addr& remote_address);

        /// Map Type: (key) ACE_INET_Addr to (value) SimpleUdpDataLink_rch
        typedef ACE_Hash_Map_Manager_Ex
                               <ACE_INET_Addr,
                                SimpleUdpDataLink_rch,
                                ACE_Hash<ACE_INET_Addr>,
                                ACE_Equal_To<ACE_INET_Addr>,
                                ACE_Null_Mutex>              AddrLinkMap;

        typedef ACE_SYNCH_MUTEX         LockType;
        typedef ACE_Guard<LockType>     GuardType;
        typedef ACE_Condition<LockType> ConditionType;

        /// Our configuration object, supplied to us in config_i().
        SimpleUdpConfiguration_rch udp_config_;

        /// This is the map of connected DataLinks.
        AddrLinkMap links_;

        /// This lock is used to protect the links_ data member.
        LockType links_lock_;

        /// We need the reactor to tell our socket to handle_input().
        TransportReactorTask_rch reactor_task_;

        /// The TransportReceiveStrategy object for this TransportImpl.
        TransportReceiveStrategy_rch receive_strategy_;

        /// There is just one SimpleUdpSocket object per SimpleUdpTransport
        /// object, and is shared amongst all SimpleUdpDataLink objects
        /// created by this SimpleUdpTransport object.
        SimpleUdpSocket_rch socket_;

        ACE_INET_Addr local_address_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleUdpTransport.inl"
#endif /* __ACE_INLINE__ */


#endif  /* TAO_DCPS_SIMPLEUDPTRANSPORT_H */
