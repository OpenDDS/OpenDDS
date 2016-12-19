/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_PERCONNECTIONSYNCH_H
#define OPENDDS_DCPS_PERCONNECTIONSYNCH_H

#include "ThreadSynch.h"

#include "ace/Condition_T.h"
#include "ace/Condition_Thread_Mutex.h"
#include "ace/Synch_Traits.h"
#include "ace/Task.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class PerConnectionSynch : public ACE_Task_Base,
      public ThreadSynch {
public:

  PerConnectionSynch(ThreadSynchResource* synch_resource, long priority, int scheduler);
  virtual ~PerConnectionSynch();

  virtual void work_available();

  virtual int open(void*);
  virtual int svc();
  virtual int close(u_long);

  OPENDDS_POOL_ALLOCATION_FWD

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
  long          dds_priority_;
  long          scheduler_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "PerConnectionSynch.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_THREADSYNCH_H */
