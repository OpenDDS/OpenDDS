#ifndef dds_DCPS_PoolAllocationBase_h
#define dds_DCPS_PoolAllocationBase_h

#include <new>
#include "SafetyProfilePool.h"

namespace OpenDDS {
  namespace DCPS {

      // TODO:  Exception behavior.
#define OPENDDS_POOL_ALLOCATION_HOOKS \
      void* operator new(size_t size) { \
        void* ptr = ACE_Allocator::instance()->malloc(size); \
        if (ptr == 0) {                                                 \
          throw std::bad_alloc();                                       \
        }                                                               \
        return ptr;                                                     \
      }                                                                 \
      void operator delete(void* ptr) { ACE_Allocator::instance()->free(ptr); } \
\
      void* operator new(size_t size, const std::nothrow_t&) { return ACE_Allocator::instance()->malloc(size); } \
      void operator delete(void* ptr, const std::nothrow_t&) { ACE_Allocator::instance()->free(ptr); } \
\
      void* operator new(size_t, void* ptr) { return ptr; } \
      void operator delete(void*, void*) {} \
\
      void* operator new[](size_t size) { \
        void*ptr = ACE_Allocator::instance()->malloc(size); \
        if (ptr == 0) {                                                 \
          throw std::bad_alloc();                                       \
        }                                                               \
        return ptr;                                                     \
      }                                                                 \
      void operator delete[](void* ptr) { ACE_Allocator::instance()->free(ptr); }

#define OPENDDS_POOL_ALLOCATION_FWD \
      void* operator new(size_t size) { return PoolAllocationBase::operator new(size); } \
      void operator delete(void* ptr) { PoolAllocationBase::operator delete(ptr); } \
\
      void* operator new(size_t size, const std::nothrow_t&) { return PoolAllocationBase::operator new(size, std::nothrow); } \
      void operator delete(void* ptr, const std::nothrow_t&) { PoolAllocationBase::operator delete(ptr, std::nothrow); } \
\
      void* operator new(size_t size, void* ptr) { return PoolAllocationBase::operator new(size, ptr); } \
      void operator delete(void* a, void* b) { PoolAllocationBase::operator delete(a, b); } \
\
      void* operator new[](size_t size) { return PoolAllocationBase::operator new[](size); } \
      void operator delete[](void* ptr) { PoolAllocationBase::operator delete[](ptr); }

    class PoolAllocationBase
    {
    public:
      OPENDDS_POOL_ALLOCATION_HOOKS
    };
  }
}

#endif /* dds_DCPS_PoolAllocationBase_h */
