// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_NULLSYNCHSTRATEGY_H
#define OPENDDS_DCPS_NULLSYNCHSTRATEGY_H

#include "dds/DCPS/dcps_export.h"
#include "ThreadSynchStrategy.h"


namespace OpenDDS
{
  namespace DCPS
  {

    class OpenDDS_Dcps_Export NullSynchStrategy : public ThreadSynchStrategy
    {
      public:

        NullSynchStrategy();
        virtual ~NullSynchStrategy();

        virtual ThreadSynch* create_synch_object(
                               ThreadSynchResource* synch_resource,
                               long                 priority,
                               int                  scheduler
                             );
    };

  } /* namespace DCPS */
} /* namespace OpenDDS */

#endif  /* OPENDDS_DCPS_NULLSYNCHSTRATEGY_H */
