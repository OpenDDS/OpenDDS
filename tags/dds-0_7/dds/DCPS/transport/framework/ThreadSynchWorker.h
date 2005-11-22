// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_THREADSYNCHWORKER_H
#define TAO_DCPS_THREADSYNCHWORKER_H

namespace TAO
{
  namespace DCPS
  {

    class ThreadSynchWorker
    {
      public:

        virtual ~ThreadSynchWorker();

        enum WorkOutcome
        {
          WORK_OUTCOME_MORE_TO_DO,
          WORK_OUTCOME_NO_MORE_TO_DO,
          WORK_OUTCOME_ClOGGED_RESOURCE,
          WORK_OUTCOME_BROKEN_RESOURCE
        };

        virtual WorkOutcome perform_work() = 0;


      protected:

        ThreadSynchWorker();
    };

  } /* namespace DCPS */
} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "ThreadSynchWorker.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_THREADSYNCHWORKER_H */
