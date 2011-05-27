// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_SIMPLEUDPCONFIGURATION_H
#define OPENDDS_DCPS_SIMPLEUDPCONFIGURATION_H

#include "SimpleUnreliableDgram_export.h"
#include "SimpleUnreliableDgramConfiguration.h"
#include "dds/DCPS/transport/framework/TransportDefs.h"
#include "ace/Configuration.h"


namespace OpenDDS
{
  namespace DCPS
  {

    class SimpleUnreliableDgram_Export SimpleUdpConfiguration 
      : public SimpleUnreliableDgramConfiguration
    {
      public:

        SimpleUdpConfiguration();
        virtual ~SimpleUdpConfiguration();

        virtual int load (const TransportIdType& id, 
                          ACE_Configuration_Heap& cf);
    };

  } /* namespace DCPS */

} /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "SimpleUdpConfiguration.inl"
#endif /* __ACE_INLINE__ */


#endif  /* OPENDDS_DCPS_SIMPLEUDPCONFIGURATION_H */
