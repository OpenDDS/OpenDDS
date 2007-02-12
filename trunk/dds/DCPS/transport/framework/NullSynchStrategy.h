// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_NULLSYNCHSTRATEGY_H
#define TAO_DCPS_NULLSYNCHSTRATEGY_H

#include "ThreadSynchStrategy.h"


namespace TAO
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
} /* namespace TAO */

#endif  /* TAO_DCPS_NULLSYNCHSTRATEGY_H */
