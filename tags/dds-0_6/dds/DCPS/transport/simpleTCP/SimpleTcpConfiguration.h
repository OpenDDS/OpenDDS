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

        /// Describes the local endpoint to be used to accept
        /// passive connections.
        ACE_INET_Addr local_address_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleTcpConfiguration.inl"
#endif /* __ACE_INLINE__ */


#endif  /* TAO_DCPS_SIMPLETCPCONFIGURATION_H */
