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
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleUdpConfiguration.inl"
#endif /* __ACE_INLINE__ */


#endif  /* TAO_DCPS_SIMPLEUDPCONFIGURATION_H */
