#ifndef dds_DCPS_PoolAllocationBase_h
#define dds_DCPS_PoolAllocationBase_h

#include <new>
#include "SafetyProfilePool.h"

namespace OpenDDS {
  namespace DCPS {

      // TODO:  Exception behavior.
#define POOL_ALLOCATION_HOOKS \
      void* operator new(size_t size) { return OpenDDS::DCPS::SafetyProfilePool::instance()->malloc(size); } \
      void operator delete(void* ptr) { OpenDDS::DCPS::SafetyProfilePool::instance()->free(ptr); } \
\
      void* operator new(size_t size, const std::nothrow_t&) { return OpenDDS::DCPS::SafetyProfilePool::instance()->malloc(size); } \
      void operator delete(void* ptr, const std::nothrow_t&) { OpenDDS::DCPS::SafetyProfilePool::instance()->free(ptr); } \
\
      void* operator new(size_t, void* ptr) { return ptr; } \
\
      void* operator new[](size_t size) { return OpenDDS::DCPS::SafetyProfilePool::instance()->malloc(size); } \
      void operator delete[](void* ptr) { OpenDDS::DCPS::SafetyProfilePool::instance()->free(ptr); }

    class PoolAllocationBase
    {
    public:
      POOL_ALLOCATION_HOOKS
    };
  }
}

#endif /* dds_DCPS_PoolAllocationBase_h */
