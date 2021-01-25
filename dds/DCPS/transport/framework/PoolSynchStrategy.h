/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_POOLSYNCHSTRATEGY_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_POOLSYNCHSTRATEGY_H

#include "ThreadSynchStrategy.h"

#include <dds/DCPS/dcps_export.h>
#include <dds/DCPS/ConditionVariable.h>

#include <ace/Synch_Traits.h>
#include <ace/Task.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class OpenDDS_Dcps_Export PoolSynchStrategy : public ACE_Task_Base,
      public ThreadSynchStrategy {
public:

  PoolSynchStrategy();
  virtual ~PoolSynchStrategy();

  virtual ThreadSynch* create_synch_object(
    ThreadSynchResource* synch_resource,
    long                 priority,
    int                  scheduler);

  virtual int open(void*);
  virtual int svc();
  virtual int close(u_long);

  void operator delete (void* ptr) { ThreadSynchStrategy::operator delete(ptr); }
private:

  typedef ACE_SYNCH_MUTEX LockType;
  typedef ACE_Guard<LockType> GuardType;
  typedef ConditionVariable<LockType> ConditionVariableType;

  LockType      lock_;
  ConditionVariableType condition_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "PoolSynchStrategy.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_POOLSYNCHSTRATEGY_H */
