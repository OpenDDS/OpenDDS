// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_PERCONNECTIONSYNCHSTRATEGY_H
#define TAO_DCPS_PERCONNECTIONSYNCHSTRATEGY_H

#include  "dds/DCPS/dcps_export.h"
#include  "ThreadSynchStrategy.h"


namespace TAO
{
  namespace DCPS
  {

    class TAO_DdsDcps_Export PerConnectionSynchStrategy : public ThreadSynchStrategy
    {
      public:

        PerConnectionSynchStrategy();
        virtual ~PerConnectionSynchStrategy();

        virtual ThreadSynch* create_synch_object
                                     (ThreadSynchResource* synch_resource);
    };

  } /* namespace DCPS */
} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "PerConnectionSynchStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_PERCONNECTIONSYNCHSTRATEGY_H */
