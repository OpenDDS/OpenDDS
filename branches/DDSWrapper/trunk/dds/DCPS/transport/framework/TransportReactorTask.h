// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_TRANSPORTREACTORTASK_H
#define OPENDDS_DCPS_TRANSPORTREACTORTASK_H


#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/RcObject_T.h"
#include "ace/Condition_T.h"
#include "ace/Task.h"

class ACE_Reactor;


namespace OpenDDS
{

  namespace DCPS
  {

    class OpenDDS_Dcps_Export TransportReactorTask : public virtual ACE_Task_Base,
                                 public virtual RcObject<ACE_SYNCH_MUTEX>
    {
      public:

        TransportReactorTask();
        virtual ~TransportReactorTask();

        virtual int open(void*);
        virtual int svc();
        virtual int close(u_long flags = 0);

        void stop();

        ACE_Reactor* get_reactor();
        const ACE_Reactor* get_reactor() const;


      private:

        typedef ACE_SYNCH_MUTEX         LockType;
        typedef ACE_Guard<LockType>     GuardType;
        typedef ACE_Condition<LockType> ConditionType;

        enum State { STATE_NOT_RUNNING, STATE_OPENING, STATE_RUNNING };

        LockType      lock_;
        State         state_;
        ConditionType condition_;
        ACE_Reactor*  reactor_;
    };

  }

}

#if defined (__ACE_INLINE__)
#include "TransportReactorTask.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTREACTORTASK_H */
