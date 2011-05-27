// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUDPCONFIGURATION_H
#define TAO_DCPS_SIMPLEUDPCONFIGURATION_H

#include  "SimpleUdp_export.h"
#include  "dds/DCPS/transport/framework/TransportConfiguration.h"
#include  "ace/INET_Addr.h"


namespace TAO
{
  namespace DCPS
  {

    class SimpleUdp_Export SimpleUdpConfiguration
                                          : public TransportConfiguration
    {
      public:

        SimpleUdpConfiguration();
        virtual ~SimpleUdpConfiguration();

        virtual int load (const TransportIdType& id, 
                          ACE_Configuration_Heap& cf);

        /// Describes the local endpoint.
        ACE_INET_Addr local_address_;

        /// Maximum period (in milliseconds) of not being able to send queued
        /// messages. If there are samples queued and no output for longer
        /// than this period then the socket will be closed and on_*_lost()
        /// callbacks will be called. If the value is zero, the default, then
        /// this check will not be made.
        int max_output_pause_period_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleUdpConfiguration.inl"
#endif /* __ACE_INLINE__ */


#endif  /* TAO_DCPS_SIMPLEUDPCONFIGURATION_H */
