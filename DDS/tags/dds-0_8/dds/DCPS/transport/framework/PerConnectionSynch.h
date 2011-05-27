// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_PERCONNECTIONSYNCH_H
#define TAO_DCPS_PERCONNECTIONSYNCH_H

#include  "ThreadSynch.h"
#include  "ace/Task.h"
#include  "ace/Synch.h"
#include  "ace/Condition_T.h"


namespace TAO
{
  namespace DCPS
  {

    class PerConnectionSynch : public ACE_Task_Base,
                               public ThreadSynch
    {
      public:

        PerConnectionSynch(ThreadSynchResource* synch_resource);
        virtual ~PerConnectionSynch();

        virtual void work_available();

        virtual int open(void*);
        virtual int svc();
        virtual int close(u_long);


      protected:

        virtual int register_worker_i();
        virtual void unregister_worker_i();


      private:

        typedef ACE_SYNCH_MUTEX         LockType;
        typedef ACE_Guard<LockType>     GuardType;
        typedef ACE_Condition<LockType> ConditionType;

        LockType      lock_;
        ConditionType condition_;
        int           work_available_;
        int           shutdown_;
    };

  } /* namespace DCPS */
} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "PerConnectionSynch.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_THREADSYNCH_H */
