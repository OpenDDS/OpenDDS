#ifndef OPENDDS_DCPS_SAFETY_PROFILE_POOL_H
#define OPENDDS_DCPS_SAFETY_PROFILE_POOL_H

#include "ace/Malloc_Base.h"

namespace OpenDDS {
namespace DCPS {

/// Memory pool for use when the Safety Profile is enabled.
///
/// Saftey Profile disallows std::free() and the delete operators
/// See PoolAllocator.h for a class that allows STL containers to use an
/// instance of SafetyProfilePool managed by our Service_Participant singleton.
///
/// This class currently allocates from a single array and doesn't attempt to
/// reuse a free()'d block.  That's expected to change as Safety Profile
/// work is completed and becomes ready for production use.
class SafetyProfilePool : public ACE_Allocator
{
public:
  explicit SafetyProfilePool(size_t size = 10*1024*1024)
    : size_(size)
    , pool_(new char[size])
    , iter_(pool_)
  {
  }

  ~SafetyProfilePool()
  {
#ifndef OPENDDS_SAFETY_PROFILE
    delete[] pool_;
#endif
  }

  void* malloc(std::size_t n)
  {
    if (iter_ + n > pool_ + size_) {
      return 0;
    }
    void* alloc = iter_;
    iter_ += n;
    return alloc;
  }

  void free(void*)
  {
  }

  void* calloc(std::size_t, char = '\0') { return 0; }
  void* calloc(std::size_t, std::size_t, char = '\0') { return 0; }
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

private:
  SafetyProfilePool(const SafetyProfilePool&);
  SafetyProfilePool& operator=(const SafetyProfilePool&);

  const size_t size_;
  char* const pool_;
  char* iter_;
};

}}
#endif // OPENDDS_DCPS_SAFETY_PROFILE_POOL_H
