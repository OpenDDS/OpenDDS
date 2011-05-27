// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_PERCONNECTIONSYNCHSTRATEGY_H
#define OPENDDS_DCPS_PERCONNECTIONSYNCHSTRATEGY_H

#include "dds/DCPS/dcps_export.h"
#include "ThreadSynchStrategy.h"


namespace OpenDDS
{
  namespace DCPS
  {

    class OpenDDS_Dcps_Export PerConnectionSynchStrategy : public ThreadSynchStrategy
    {
      public:

        PerConnectionSynchStrategy();
        virtual ~PerConnectionSynchStrategy();

        virtual ThreadSynch* create_synch_object
                                     (ThreadSynchResource* synch_resource);
    };

  } /* namespace DCPS */
} /* namespace OpenDDS */

#if defined (__ACE_INLINE__)
#include "PerConnectionSynchStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_PERCONNECTIONSYNCHSTRATEGY_H */
