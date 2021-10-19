#ifndef OPENDDS_DCPS_POOL_ALLOCATOR_H
#define OPENDDS_DCPS_POOL_ALLOCATOR_H

#include <ace/config-macros.h>
#if defined OPENDDS_SAFETY_PROFILE && defined ACE_HAS_ALLOC_HOOKS
#  define OPENDDS_POOL_ALLOCATOR 1
#else
#  define OPENDDS_POOL_ALLOCATOR 0
#endif

#if OPENDDS_POOL_ALLOCATOR
#  include "dcps_export.h"
#  include "SafetyProfilePool.h"
#endif
#include <dds/Versioned_Namespace.h>

#include <limits>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <queue>
#include <set>
#ifdef ACE_HAS_CPP11
#include <unordered_map>
#endif

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#if OPENDDS_POOL_ALLOCATOR

OpenDDS_Dcps_Export void* pool_alloc_memory(size_t size);

OpenDDS_Dcps_Export void pool_free_memory(void* ptr);

/// Adapt the MemoryPool for use with STL containers.
///
/// See Definitions.h for macros that assist with instantiating STL containers
/// with switchable support for the allocator (when Safety Profile is enabled).
///
/// This class template models the C++03 Allocator concept and is stateless.
template <typename T>
class OpenDDS_Dcps_Export PoolAllocator
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
    void* raw_mem = ACE_Allocator::instance()->malloc(n * sizeof(T));
    if (!raw_mem) throw std::bad_alloc();
    return static_cast<T*>(raw_mem);
  }

  static void deallocate(T* ptr, std::size_t)
  {
    ACE_Allocator::instance()->free(ptr);
  }

  static void construct(T* ptr, const T& value)
  {
    new (static_cast<void*>(ptr)) T(value);
  }

#ifdef ACE_HAS_CPP11
  static void construct(T* ptr, T&& value)
  {
    new (static_cast<void*>(ptr)) T(std::move(value));
  }
#endif

  static void destroy(T* ptr)
  {
    ptr->~T();
  }

  static size_type max_size()
  {
    return std::numeric_limits<size_type>::max();
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

template <typename T>
struct add_const { typedef const T type; };

// TODO(iguessthislldo): Rewrite with using in C++11
#define OPENDDS_ALLOCATOR(T) OpenDDS::DCPS::PoolAllocator<T >
typedef std::basic_string<char, std::char_traits<char>, OPENDDS_ALLOCATOR(char)> String;
typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, OPENDDS_ALLOCATOR(wchar_t)>
  WString;
#define OPENDDS_MAP(K, V) std::map<K, V, std::less<K >, \
          OpenDDS::DCPS::PoolAllocator<std::pair<OpenDDS::DCPS::add_const<K >::type, V > > >
#define OPENDDS_MAP_CMP(K, V, C) std::map<K, V, C, \
          OpenDDS::DCPS::PoolAllocator<std::pair<OpenDDS::DCPS::add_const<K >::type, V > > >
#define OPENDDS_MULTIMAP(K, T) std::multimap<K, T, std::less<K >, \
          OpenDDS::DCPS::PoolAllocator<std::pair<OpenDDS::DCPS::add_const<K >::type, T > > >
#define OPENDDS_MULTIMAP_CMP(K, T, C) std::multimap<K, T, C, \
          OpenDDS::DCPS::PoolAllocator<std::pair<OpenDDS::DCPS::add_const<K >::type, T > > >
#define OPENDDS_MAP_T(K, V) std::map<K, V, std::less<K >, \
          OpenDDS::DCPS::PoolAllocator<std::pair<typename OpenDDS::DCPS::add_const<K >::type, V > > >
#define OPENDDS_MAP_CMP_T(K, V, C) std::map<K, V, C, \
          OpenDDS::DCPS::PoolAllocator<std::pair<typename OpenDDS::DCPS::add_const<K >::type, V > > >
#define OPENDDS_MULTIMAP_T(K, T) std::multimap<K, T, std::less<K >, \
          OpenDDS::DCPS::PoolAllocator<std::pair<typename OpenDDS::DCPS::add_const<K >::type, T > > >
#define OPENDDS_MULTIMAP_CMP_T(K, T, C) std::multimap<K, T, C, \
          OpenDDS::DCPS::PoolAllocator<std::pair<typename OpenDDS::DCPS::add_const<K >::type, T > > >
#define OPENDDS_SET(K) std::set<K, std::less<K >, \
          OpenDDS::DCPS::PoolAllocator<K > >
#define OPENDDS_SET_CMP(K, C) std::set<K, C, \
          OpenDDS::DCPS::PoolAllocator<K > >
#define OPENDDS_MULTISET(K) std::multiset<K, std::less<K >, \
          OpenDDS::DCPS::PoolAllocator<K > >
#define OPENDDS_MULTISET_CMP(K, C) std::multiset<K, C, \
          OpenDDS::DCPS::PoolAllocator<K > >
#define OPENDDS_VECTOR(T) std::vector<T, \
          OpenDDS::DCPS::PoolAllocator<T > >
#define OPENDDS_LIST(T) std::list<T, \
          OpenDDS::DCPS::PoolAllocator<T > >
#define OPENDDS_DEQUE(T) std::deque<T, \
          OpenDDS::DCPS::PoolAllocator<T > >
#define OPENDDS_QUEUE(T) std::queue<T, std::deque<T, \
          OpenDDS::DCPS::PoolAllocator<T > > >
#ifdef ACE_HAS_CPP11
#define OPENDDS_UNORDERED_MAP(K, V) std::unordered_map<K, V, std::hash<K >, std::equal_to<K >, \
          OpenDDS::DCPS::PoolAllocator<std::pair<OpenDDS::DCPS::add_const<K >::type, V > > >
#define OPENDDS_UNORDERED_MAP_CHASH(K, V, C) std::unordered_map<K, V, C, std::equal_to<K >, \
          OpenDDS::DCPS::PoolAllocator<std::pair<OpenDDS::DCPS::add_const<K >::type, V > > >
#define OPENDDS_UNORDERED_MAP_T(K, V) std::unordered_map<K, V, std::hash<K >, std::equal_to<K >, \
          OpenDDS::DCPS::PoolAllocator<std::pair<typename OpenDDS::DCPS::add_const<K >::type, V > > >
#define OPENDDS_UNORDERED_MAP_CHASH_T(K, V, C) std::unordered_map<K, V, C, std::equal_to<K >, \
          OpenDDS::DCPS::PoolAllocator<std::pair<typename OpenDDS::DCPS::add_const<K >::type, V > > >
#endif

#else // (!OPENDDS_POOL_ALLOCATOR)
#define OPENDDS_ALLOCATOR(T) std::allocator<T >
typedef std::string String;
typedef std::wstring WString;
#define OPENDDS_MAP(K, V) std::map<K, V >
#define OPENDDS_MAP_CMP(K, V, C) std::map<K, V, C >
#define OPENDDS_MULTIMAP(K, T) std::multimap<K, T >
#define OPENDDS_MULTIMAP_CMP(K, T, C) std::multimap<K, T, C >
#define OPENDDS_MAP_T OPENDDS_MAP
#define OPENDDS_MAP_CMP_T OPENDDS_MAP_CMP
#define OPENDDS_MULTIMAP_T OPENDDS_MULTIMAP
#define OPENDDS_MULTIMAP_CMP_T OPENDDS_MULTIMAP_CMP
#define OPENDDS_SET(K) std::set<K >
#define OPENDDS_SET_CMP(K, C) std::set<K, C >
#define OPENDDS_MULTISET(K) std::multiset<K >
#define OPENDDS_MULTISET_CMP(K, C) std::multiset<K, C >
#define OPENDDS_VECTOR(T) std::vector<T >
#define OPENDDS_LIST(T) std::list<T >
#define OPENDDS_DEQUE(T) std::deque<T >
#define OPENDDS_QUEUE(T) std::queue<T >
#ifdef ACE_HAS_CPP11
#define OPENDDS_UNORDERED_MAP(K, V) std::unordered_map<K, V >
#define OPENDDS_UNORDERED_MAP_CHASH(K, V, C) std::unordered_map<K, V, C >
#define OPENDDS_UNORDERED_MAP_T OPENDDS_UNORDERED_MAP
#define OPENDDS_UNORDERED_MAP_CHASH_T OPENDDS_UNORDERED_MAP_CHASH
#endif

#endif // OPENDDS_POOL_ALLOCATOR

#define OPENDDS_STRING OpenDDS::DCPS::String
#define OPENDDS_WSTRING OpenDDS::DCPS::WString

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_POOL_ALLOCATOR_H
