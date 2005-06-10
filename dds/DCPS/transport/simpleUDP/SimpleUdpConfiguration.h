// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEUDPCONFIGURATION_H
#define TAO_DCPS_SIMPLEUDPCONFIGURATION_H

#include  "dds/DCPS/dcps_export.h"
#include  "dds/DCPS/transport/framework/TransportConfiguration.h"
#include  "ace/INET_Addr.h"


namespace TAO
{
  namespace DCPS
  {

    class TAO_DdsDcps_Export SimpleUdpConfiguration
                                          : public TransportConfiguration
    {
      public:

        SimpleUdpConfiguration();
        virtual ~SimpleUdpConfiguration();

        /// Describes the local endpoint.
        ACE_INET_Addr local_address_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleUdpConfiguration.inl"
#endif /* __ACE_INLINE__ */


#endif  /* TAO_DCPS_SIMPLEUDPCONFIGURATION_H */
