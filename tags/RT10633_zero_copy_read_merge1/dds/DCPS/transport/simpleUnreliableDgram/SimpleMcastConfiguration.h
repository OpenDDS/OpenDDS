// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEMCASTCONFIGURATION_H
#define TAO_DCPS_SIMPLEMCASTCONFIGURATION_H

#include "SimpleUnreliableDgram_export.h"
#include "SimpleUnreliableDgramConfiguration.h"
#include "ace/INET_Addr.h"


namespace TAO
{
  namespace DCPS
  {

    class SimpleUnreliableDgram_Export SimpleMcastConfiguration
                                          : public SimpleUnreliableDgramConfiguration
    {
      public:

        SimpleMcastConfiguration();
        virtual ~SimpleMcastConfiguration();

        virtual int load (const TransportIdType& id, 
                          ACE_Configuration_Heap& cf);

        /// This is the multicast group to send/receive on.
        ACE_INET_Addr multicast_group_address_;

        /// Are we a receiver?
        bool receiver_;
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleMcastConfiguration.inl"
#endif /* __ACE_INLINE__ */


#endif  /* TAO_DCPS_SIMPLEMCASTCONFIGURATION_H */
