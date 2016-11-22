/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef ZEROCOPYALLOCATOR_H
#define ZEROCOPYALLOCATOR_H

#include /**/ "ace/pre.h"
#include "ace/Malloc_Base.h"          /* Need ACE_Allocator */
// not needed export for templates #include "dcps_export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/Versioned_Namespace.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// This allocator is "Fast" because it's pool can be on the stack
///   (If the object is on the stack and hence it does not require the
///    cost of allocating and deallocating on the heap.  It object is on the
///    heap then it requires just one allocation; not two.)
/// @WARNING The object using this allocator must not have a scope smaller than this object !!!
template <class T, std::size_t N>
class FirstTimeFastAllocator : public ACE_Allocator {
public:
  FirstTimeFastAllocator();
  virtual void *malloc(size_t nbytes);
  virtual void free(void *ptr);

  /// These methods are no-ops.
  virtual void *calloc(size_t nbytes, char initial_value = '\0');
  virtual void *calloc(size_t n_elem, size_t elem_size, char initial_value = '\0');
  virtual int remove();
  virtual int bind(const char *name, void *pointer, int duplicates = 0);
  virtual int trybind(const char *name, void *&pointer);
  virtual int find(const char *name, void *&pointer);
  virtual int find(const char *name);
  virtual int unbind(const char *name);
  virtual int unbind(const char *name, void *&pointer);
  virtual int sync(ssize_t len = -1, int flags = MS_SYNC);
  virtual int sync(void *addr, size_t len, int flags = MS_SYNC);
  virtual int protect(ssize_t len = -1, int prot = PROT_RDWR);
  virtual int protect(void *addr, size_t len, int prot = PROT_RDWR);
#if defined (ACE_HAS_MALLOC_STATS)
  virtual void print_stats() const;
#endif /* ACE_HAS_MALLOC_STATS */
  virtual void dump() const;

  T* pool() {
    return pool_;
  }

private:
  /// is this the first time this is allocated?
  bool firstTime_;

  /// the pool of allocated memory.
  T pool_[N];
};

} // namespace  DDS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "dds/DCPS/ZeroCopyAllocator_T.inl"
#endif /* __ACE_INLINE__ */

#if defined (ACE_TEMPLATES_REQUIRE_SOURCE)
#include "dds/DCPS/ZeroCopyAllocator_T.cpp"
#endif /* ACE_TEMPLATES_REQUIRE_SOURCE */

#if defined (ACE_TEMPLATES_REQUIRE_PRAGMA)
#pragma implementation ("ZeroCopyAllocator_T.cpp")
#endif /* ACE_TEMPLATES_REQUIRE_PRAGMA */

#include /**/ "ace/post.h"

#endif /* ZEROCOPYALLOCATOR_H  */
