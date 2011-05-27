// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUNRELIABLEDGRAMCONFIGURATION_H
#define TAO_DCPS_SIMPLEUNRELIABLEDGRAMCONFIGURATION_H

#include  "SimpleUnreliableDgram_export.h"
#include  "dds/DCPS/transport/framework/TransportConfiguration.h"
#include  "ace/INET_Addr.h"


namespace TAO
{
  namespace DCPS
  {

    class SimpleUnreliableDgram_Export SimpleUnreliableDgramConfiguration
                                          : public TransportConfiguration
    {
      public:

        SimpleUnreliableDgramConfiguration();
        virtual ~SimpleUnreliableDgramConfiguration();

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
#include "SimpleUnreliableDgramConfiguration.inl"
#endif /* __ACE_INLINE__ */


#endif  /* TAO_DCPS_SIMPLEUNRELIABLEDGRAMCONFIGURATION_H */
