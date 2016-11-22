#ifndef dds_DCPS_PoolAllocationBase_h
#define dds_DCPS_PoolAllocationBase_h

#include <new>
#include "SafetyProfilePool.h"

#define OPENDDS_POOL_ALLOCATION_HOOKS \
  void* operator new(size_t size)                                       \
  {                                                                     \
    void* const ptr = ACE_Allocator::instance()->malloc(size);          \
    if (ptr == 0) {                                                     \
      throw std::bad_alloc();                                           \
    }                                                                   \
    return ptr;                                                         \
  }                                                                     \
                                                                        \
  void operator delete(void* ptr)                                       \
  { ACE_Allocator::instance()->free(ptr); }                             \
                                                                        \
  void* operator new(size_t size, const std::nothrow_t&) throw()        \
  { return ACE_Allocator::instance()->malloc(size); }                   \
                                                                        \
  void operator delete(void* ptr, const std::nothrow_t&)                \
  { ACE_Allocator::instance()->free(ptr); }                             \
                                                                        \
  void* operator new(size_t, void* ptr) { return ptr; }                 \
                                                                        \
  void operator delete(void*, void*) {}                                 \
                                                                        \
  void* operator new[](size_t size)                                     \
  {                                                                     \
    void* const ptr = ACE_Allocator::instance()->malloc(size);          \
    if (ptr == 0) {                                                     \
      throw std::bad_alloc();                                           \
    }                                                                   \
    return ptr;                                                         \
  }                                                                     \
                                                                        \
  void operator delete[](void* ptr)                                     \
  { ACE_Allocator::instance()->free(ptr); }                             \
                                                                        \
  void* operator new[](size_t size, const std::nothrow_t&) throw()      \
  { return ACE_Allocator::instance()->malloc(size); }                   \
                                                                        \
  void operator delete[](void* ptr, const std::nothrow_t&)              \
  { ACE_Allocator::instance()->free(ptr); }                             \


#define OPENDDS_POOL_ALLOCATION_FWD            \
  using PoolAllocationBase::operator new;      \
  using PoolAllocationBase::operator new[];    \
  using PoolAllocationBase::operator delete;   \
  using PoolAllocationBase::operator delete[]; \

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class PoolAllocationBase
{
public:
  OPENDDS_POOL_ALLOCATION_HOOKS
};

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* dds_DCPS_PoolAllocationBase_h */
