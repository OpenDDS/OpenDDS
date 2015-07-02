/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"  ////Only the _pch include should start with DCPS/
#include "SafetyProfilePool.h"
#include "dds/DCPS/debug.h"
#include <stdexcept>

#ifdef OPENDDS_SAFETY_PROFILE
namespace OpenDDS {  namespace DCPS {

SafetyProfilePool::SafetyProfilePool()
: main_pool_(0)
{
}

SafetyProfilePool::~SafetyProfilePool()
{
  // Never delete, because this is always a SAFETY_PROFILE build
  //delete main_pool_;
}

void
SafetyProfilePool::configure_pool(size_t size, size_t granularity)
{
  ACE_GUARD(ACE_Thread_Mutex, lock, lock_);

  if (main_pool_ == NULL) {
    main_pool_ = new MemoryPool(size, granularity);
  }
}

void
SafetyProfilePool::install()
{
  if (ACE_Allocator::instance () != this) {
    ACE_Allocator::instance (this);
  }
}

SafetyProfilePool*
SafetyProfilePool::instance() {
  return instance_;
}

SafetyProfilePool* SafetyProfilePool::instance_ = 0;

struct InstanceMaker {
  InstanceMaker() {
    SafetyProfilePool::instance_ = new SafetyProfilePool();
  }

  ~InstanceMaker() {
    if (DCPS_debug_level) {
      if (SafetyProfilePool::instance_->main_pool_) {
        ACE_DEBUG((LM_INFO, "LWM: main pool: %d bytes\n",
                   SafetyProfilePool::instance_->main_pool_->lwm_free_bytes()));
      }
    }
  }

};

InstanceMaker instance_maker_;

}}

#endif
