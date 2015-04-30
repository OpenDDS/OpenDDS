#include "SafetyProfilePool.h"
#include "dds/DCPS/debug.h"
#include <stdexcept>

namespace OpenDDS {  namespace DCPS {

SafetyProfilePool:: SafetyProfilePool()
: init_pool_(new MemoryPool(1024*1024, 8))
, main_pool_(0)
{
}

SafetyProfilePool::~SafetyProfilePool()
{
  if (DCPS_debug_level) {
    if (main_pool_) {
      ACE_DEBUG((LM_INFO, "LWM: init pool: %d bytes, main pool: %d bytes\n",
                 init_pool_->lwm_free_bytes(), main_pool_->lwm_free_bytes()));
    } else {
      ACE_DEBUG((LM_INFO, "LWM: init pool: %d bytes\n",
                 init_pool_->lwm_free_bytes()));
    }
  }

#ifndef OPENDDS_SAFETY_PROFILE
  delete init_pool_;
  delete main_pool_;
#endif
}

void
SafetyProfilePool::configure_pool(size_t size, size_t granularity)
{
  ACE_GUARD(ACE_Thread_Mutex, lock, lock_);

  if (main_pool_ == NULL) {
    main_pool_ = new MemoryPool(size, granularity);
  }
}

SafetyProfilePool*
SafetyProfilePool::instance() {
  return ACE_Singleton<SafetyProfilePool, ACE_SYNCH_MUTEX>::instance();
}

}}

