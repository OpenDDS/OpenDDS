// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLETCPCONFIGURATION_H
#define TAO_DCPS_SIMPLETCPCONFIGURATION_H

#include  "dds/DCPS/dcps_export.h"
#include  "dds/DCPS/transport/framework/TransportConfiguration.h"
#include  "ace/INET_Addr.h"


namespace TAO
{
  namespace DCPS
  {

    class TAO_DdsDcps_Export SimpleTcpConfiguration
                                          : public TransportConfiguration
    {
      public:

        SimpleTcpConfiguration();
        virtual ~SimpleTcpConfiguration();
        virtual int load (const TransportIdType& id, 
                          ACE_Configuration_Heap& cf);

        /// Describes the local endpoint to be used to accept
        /// passive connections.
        ACE_INET_Addr local_address_;

        bool enable_nagle_algorithm_;

        /// Determine if reestablishment needs be performed when the connection
        /// is lost.
        bool attempt_connection_reestablishment_;
        /// The initial retry delay in milliseconds. 
        int conn_retry_initial_delay_;
        /// The backoff multiplier for reconnection strategy. 
        int conn_retry_backoff_multiplier_;
        /// Number of attemps before giving up.
        int conn_retry_attempts_;
        /// The time period in milliseconds for the acceptor side of a connection
        /// to wait for reconnecting.
        int passive_reconenct_duration_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleTcpConfiguration.inl"
#endif /* __ACE_INLINE__ */


#endif  /* TAO_DCPS_SIMPLETCPCONFIGURATION_H */
