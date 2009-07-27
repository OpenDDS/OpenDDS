// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_TRANSPORTCONFIGURATION_H
#define OPENDDS_DCPS_TRANSPORTCONFIGURATION_H

#include "dds/DCPS/dcps_export.h"
#include "TransportDefs.h"
#include "dds/DCPS/RcObject_T.h"
#include "ace/Synch.h"
#include "ace/Configuration.h"
#include "ace/SString.h"

class ACE_Reactor;

namespace OpenDDS
{

  namespace DCPS
  {

    // TBD SOON - The ThreadSynchStrategy should be reference counted, and
    //            then we could do away with the mutator and accessor by
    //            moving the send_thread_strategy_ data member to the public
    //            section, and change its type from dumb to smart pointer.
    class ThreadSynchStrategy;


// TBD - REMOVE THIS WHEN ANSWERED - IT SHOULD STAY REFERENCE COUNTED.
#if 0
//MJM: Why is it necessary to ref count this class?  Isn't this just
//MJM: data?
#endif

    /**
     * @class TransportConfiguration
     *
     * @brief Base class to hold configuration settings for TransportImpls.
     *
     * Each transport implementation will need to define a concrete
     * subclass of the TransportConfiguration class.  The base
     * class (TransportConfiguration) contains configuration settings that
     * are common to all (or most) concrete transport implementations.
     * The concrete transport implementation defines any configuration
     * settings that it requires within its concrete subclass of this
     * TransportConfiguration base class.
     *
     * The TransportConfiguration object is supplied to the
     * TransportImpl::configure() method.
     */
    class OpenDDS_Dcps_Export TransportConfiguration : public RcObject<ACE_SYNCH_MUTEX>
    {
      public:

        /// Dtor
        virtual ~TransportConfiguration();

        /// Mutator for the "send thread strategy" object.  Will delete
        /// the existing strategy object (ie, the default) first.
        void send_thread_strategy(ThreadSynchStrategy* strategy);

        /// Accessor for the "send thread strategy" object.
        ThreadSynchStrategy* send_thread_strategy();

        /// Overwrite the default configurations with the configuration for the
        /// give transport_id in ACE_Configuration_Heap object.
        virtual int  load (const TransportIdType& id,
                           ACE_Configuration_Heap& config);

        /// Flag used to marshall/demarshall bytes sent/received.
        bool swap_bytes_;

        /// Number of pre-created link (list) objects per pool for the
        /// "send queue" of each DataLink.
        size_t queue_messages_per_pool_;

        /// Initial number of pre-allocated pools of link (list) objects
        /// for the "send queue" of each DataLink.
        size_t queue_initial_pools_;

        /// Max size (in bytes) of a packet (packet header + sample(s))
        ACE_UINT32 max_packet_size_;

        /// Max number of samples that should ever be in a single packet.
        size_t max_samples_per_packet_;

        /// Optimum size (in bytes) of a packet (packet header + sample(s)).
        ACE_UINT32 optimum_packet_size_;

        ACE_TString transport_type_;

        /// Flag for whether a new thread is needed for connection to
        /// send without backpressure.
        bool thread_per_connection_;

        /// Delay in milliseconds that the datalink should be released after all
        /// associations are removed. The default value is 10 seconds.
        long datalink_release_delay_;


      protected:

        /// Default ctor.
        TransportConfiguration();

        static ACE_TString id_to_section_name(const TransportIdType& id);

      private:

        /// Adjust the configuration values which gives warning on adjusted
        /// value.
        void adjust_config_value ();

        /// Thread strategy used for sending data samples (and incomplete
        /// packets) when a DataLink has encountered "backpressure".
        ThreadSynchStrategy* send_thread_strategy_;
   };

  }

}

#if defined(__ACE_INLINE__)
#include "TransportConfiguration.inl"
#endif /* __ACE_INLINE__ */

#endif
