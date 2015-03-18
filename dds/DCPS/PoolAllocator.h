#ifndef OPENDDS_DCPS_POOL_ALLOCATOR_H
#define OPENDDS_DCPS_POOL_ALLOCATOR_H

#include <string>
#include <map>
#ifdef OPENDDS_SAFETY_PROFILE
#include "dcps_export.h"
#include "SafetyProfilePool.h"

namespace OpenDDS {
namespace DCPS {
  
OpenDDS_Dcps_Export void* pool_alloc_memory(size_t size);

OpenDDS_Dcps_Export void pool_free_memory(void* ptr);

/// Adapt the Service Participant's memory pool for use with STL containers.
///
/// See Definitions.h for macros that assist with instantiating STL containers
/// with switchable support for the allocator (when Safety Profile is enabled).
///
/// This class template models the C++03 Allocator concept and is stateless.
template <typename T>
class PoolAllocator
{
public:
  typedef T value_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;
  template <typename U> struct rebind { typedef PoolAllocator<U> other; };

  PoolAllocator() {}

  template <typename U>
  PoolAllocator(const PoolAllocator<U>&) {}

  static T* allocate(std::size_t n)
  {
    void* raw_mem = SafetyProfilePool::instance()->malloc(n * sizeof(T));
    if (!raw_mem) throw std::bad_alloc();
    return static_cast<T*>(raw_mem);
  }

  static void deallocate(T* ptr, std::size_t)
  {
    SafetyProfilePool::instance()->free(ptr);
  }

#ifdef ACE_LYNXOS_MAJOR
  static void deallocate(void* ptr, std::size_t)
  {
    SafetyProfilePool::instance()->free(ptr);
  }
#endif

  static void construct(T* ptr, const T& value)
  {
    new (static_cast<void*>(ptr)) T(value);
  }

  static void destroy(T* ptr)
  {
    ptr->~T();
  }
};

template <typename T, typename U>
bool operator==(const PoolAllocator<T>&, const PoolAllocator<U>&)
{
  return true;
}

template <typename T, typename U>
bool operator!=(const PoolAllocator<T>&, const PoolAllocator<U>&)
{
  return false;
}

}}

#ifdef ACE_LYNXOS_MAJOR
#define OPENDDS_STRING std::basic_string<char, std::string_char_traits<char>, \
          OpenDDS::DCPS::PoolAllocator<char> >
#else
#define OPENDDS_STRING std::basic_string<char, std::char_traits<char>, \
          OpenDDS::DCPS::PoolAllocator<char> >
#endif // ACE_LYNXOS_MAJOR
#define OPENDDS_MAP(K, V) std::map<K, V, std::less<K>, \
          OpenDDS::DCPS::PoolAllocator<std::pair<const K, V> > >
#define OPENDDS_MAP_CMP(K, V, C) std::map<K, V, C, \
          OpenDDS::DCPS::PoolAllocator<std::pair<const K, V> > >
#else
#define OPENDDS_STRING std::string
#define OPENDDS_MAP(K, V) std::map<K, V>
#define OPENDDS_MAP_CMP(K, V, C) std::map<K, V, C>
#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_POOL_ALLOCATOR_H
