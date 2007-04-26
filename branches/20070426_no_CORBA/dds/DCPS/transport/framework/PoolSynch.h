// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_POOLSYNCH_H
#define TAO_DCPS_POOLSYNCH_H

#include "dds/DCPS/dcps_export.h"
#include "ThreadSynch.h"


namespace TAO
{
  namespace DCPS
  {

    class PoolSynchStrategy;
    class ThreadSynchResource;


    class TAO_DdsDcps_Export PoolSynch : public ThreadSynch
    {
      public:

        PoolSynch(PoolSynchStrategy* strategy,
                  ThreadSynchResource* synch_resource);
        virtual ~PoolSynch();

        virtual void work_available();


      protected:

        virtual void unregister_worker_i();


      private:

        PoolSynchStrategy* strategy_;
    };

  } /* namespace DCPS */
} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "PoolSynch.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_POOLSYNCH_H */
