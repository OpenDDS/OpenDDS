/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SAFETY_PROFILE_POOL_H
#define OPENDDS_DCPS_SAFETY_PROFILE_POOL_H

#include "ace/Malloc_Base.h"
#include /**/ "dds/Versioned_Namespace.h"

#ifdef OPENDDS_SAFETY_PROFILE
#include "ace/Atomic_Op.h"
#include "ace/Singleton.h"
#include "dcps_export.h"
#include "MemoryPool.h"

#include <cstring>

class SafetyProfilePoolTest;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// Memory pool for use when the Safety Profile is enabled.
///
/// Saftey Profile disallows std::free() and the delete operators
/// See PoolAllocator.h for a class that allows STL containers to use an
/// instance of SafetyProfilePool managed by our Service_Participant singleton.
class OpenDDS_Dcps_Export SafetyProfilePool : public ACE_Allocator
{
  friend class SafetyProfilePoolTest;
public:
  SafetyProfilePool();
  ~SafetyProfilePool();

  void configure_pool(size_t size, size_t granularity);
  void install();

  void* malloc(std::size_t size)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, lock, lock_, 0);
    return main_pool_->pool_alloc(size);
  }

  void free(void* ptr)
  {
    ACE_GUARD(ACE_Thread_Mutex, lock, lock_);
    main_pool_->pool_free(ptr);
  }

  void* calloc(std::size_t bytes, char init = '\0')
  {
    void* const mem = malloc(bytes);
    std::memset(mem, init, bytes);
    return mem;
  }

  void* calloc(std::size_t elems, std::size_t size, char init = '\0')
  {
    return calloc(elems * size, init);
  }

  int remove() { return -1; }
  int bind(const char*, void*, int = 0) { return -1; }
  int trybind(const char*, void*&) { return -1; }
  int find(const char*, void*&) { return -1; }
  int find(const char*) { return -1; }
  int unbind(const char*, void*&) { return -1; }
  int unbind(const char*) { return -1; }
  int sync(ssize_t = -1, int = MS_SYNC) { return -1; }
  int sync(void*, size_t, int = MS_SYNC) { return -1; }
  int protect(ssize_t = -1, int = PROT_RDWR) { return -1; }
  int protect(void*, size_t, int = PROT_RDWR) { return -1; }
  void dump() const {}

  /// Return a singleton instance of this class.
  static SafetyProfilePool* instance();

private:
  SafetyProfilePool(const SafetyProfilePool&);
  SafetyProfilePool& operator=(const SafetyProfilePool&);

  MemoryPool* main_pool_;
  ACE_Thread_Mutex lock_;
  static SafetyProfilePool* instance_;
  friend class InstanceMaker;
};

}}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#else // ! OPENDDS_SAFETY_PROFILE

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {
typedef ACE_Allocator SafetyProfilePool;
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_SAFETY_PROFILE

#endif // OPENDDS_DCPS_SAFETY_PROFILE_POOL_H
