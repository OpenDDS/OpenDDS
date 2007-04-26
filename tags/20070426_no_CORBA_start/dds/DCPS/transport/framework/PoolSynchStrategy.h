// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_POOLSYNCHSTRATEGY_H
#define TAO_DCPS_POOLSYNCHSTRATEGY_H

#include "dds/DCPS/dcps_export.h"
#include "ThreadSynchStrategy.h"
#include "ace/Task.h"
#include "ace/Synch.h"
#include "ace/Condition_T.h"


namespace TAO
{
  namespace DCPS
  {

    class TAO_DdsDcps_Export PoolSynchStrategy : public ACE_Task_Base,
                              public ThreadSynchStrategy
    {
      public:

        PoolSynchStrategy();
        virtual ~PoolSynchStrategy();

        virtual ThreadSynch* create_synch_object
                                       (ThreadSynchResource* synch_resource);

        virtual int open(void*);
        virtual int svc();
        virtual int close(u_long);


      private:

        typedef ACE_SYNCH_MUTEX         LockType;
        typedef ACE_Guard<LockType>     GuardType;
        typedef ACE_Condition<LockType> ConditionType;

        LockType      lock_;
        ConditionType condition_;
        int           shutdown_;
    };

  } /* namespace DCPS */
} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "PoolSynchStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_POOLSYNCHSTRATEGY_H */
