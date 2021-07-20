/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_CONDITIONIMPL_H
#define OPENDDS_DCPS_CONDITIONIMPL_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "Definitions.h"
#include "PoolAllocator.h"
#include "WaitSet.h"

#include "ace/Recursive_Thread_Mutex.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ConditionImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::Condition> {
public:
  DDS::ReturnCode_t attach_to_ws(DDS::WaitSet_ptr ws);
  DDS::ReturnCode_t detach_from_ws(DDS::WaitSet_ptr ws);
  void signal_all();

protected:
  ConditionImpl() {}
  virtual ~ConditionImpl() {}

  typedef OPENDDS_SET(OpenDDS::DCPS::WeakRcHandle<DDS::WaitSet>) WaitSetSet;
  WaitSetSet waitsets_;
  /// Use a recursive mutex because we have derived classes that
  /// need the recursive behavior
  ACE_Recursive_Thread_Mutex lock_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
