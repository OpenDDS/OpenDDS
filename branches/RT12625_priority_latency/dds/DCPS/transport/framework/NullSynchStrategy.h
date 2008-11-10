// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_NULLSYNCHSTRATEGY_H
#define OPENDDS_DCPS_NULLSYNCHSTRATEGY_H

#include "ThreadSynchStrategy.h"


namespace OpenDDS
{
  namespace DCPS
  {

    class NullSynchStrategy : public ThreadSynchStrategy
    {
      public:

        NullSynchStrategy();
        virtual ~NullSynchStrategy();

        virtual ThreadSynch* create_synch_object
                                       (ThreadSynchResource* synch_resource);
    };

  } /* namespace DCPS */
} /* namespace OpenDDS */

#endif  /* OPENDDS_DCPS_NULLSYNCHSTRATEGY_H */
