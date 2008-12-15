// -*- C++ -*-
#ifndef OPENDDS_DCPS_WAITSET_H
#define OPENDDS_DCPS_WAITSET_H

#include "dds/DdsDcpsInfrastructureC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/LocalObject.h"
#include "dds/DCPS/Definitions.h"

#include "ace/Thread_Mutex.h"
#include "ace/Condition_Recursive_Thread_Mutex.h"
#include "ace/Atomic_Op.h"

#include <set>

namespace OpenDDS
{
  namespace DCPS
  {
    class ConditionImpl;
  }
}

namespace DDS
{
  class WaitSet;
  typedef WaitSet* WaitSet_ptr;

  typedef TAO_Objref_Var_T<WaitSet> WaitSet_var;

  class OpenDDS_Dcps_Export WaitSet
    : public virtual OpenDDS::DCPS::LocalObject<WaitSetInterf>
  {
  public:
    typedef WaitSet_ptr _ptr_type;
    typedef WaitSet_var _var_type;

    WaitSet()
      : lock_(), cond_(lock_)
    {}

    virtual ~WaitSet() {}

    ReturnCode_t wait(ConditionSeq& active_conditions,
        const Duration_t& timeout)
      ACE_THROW_SPEC ((CORBA::SystemException));

    ReturnCode_t attach_condition(Condition_ptr cond)
      ACE_THROW_SPEC ((CORBA::SystemException));

    ReturnCode_t detach_condition(Condition_ptr cond)
      ACE_THROW_SPEC ((CORBA::SystemException));

    ReturnCode_t get_conditions (ConditionSeq& attached_conditions)
      ACE_THROW_SPEC ((CORBA::SystemException));

    static WaitSet_ptr _duplicate(WaitSet_ptr obj);

    typedef std::set<Condition_var,
      OpenDDS::DCPS::VarLess<Condition> > ConditionSet;

  private:
    void signal(Condition_ptr cond);
    friend class OpenDDS::DCPS::ConditionImpl;

    ACE_Recursive_Thread_Mutex lock_;
    ACE_Condition_Recursive_Thread_Mutex cond_;
    ACE_Atomic_Op<ACE_Thread_Mutex, long> waiting_;

    ConditionSet attached_conditions_;
    ConditionSet signaled_conditions_;
  };
}

namespace TAO
{
  template<>
  struct OpenDDS_Dcps_Export Objref_Traits<DDS::WaitSet>
  {
    static DDS::WaitSet_ptr duplicate(DDS::WaitSet_ptr p);
    static void release(DDS::WaitSet_ptr p);
    static DDS::WaitSet_ptr nil();
    static CORBA::Boolean marshal(const DDS::WaitSet_ptr p,
      TAO_OutputCDR & cdr);
  };
}

#endif
