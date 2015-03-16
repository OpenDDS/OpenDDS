#include "PoolAllocator.h"
#include "Service_Participant.h"

namespace OpenDDS { namespace DCPS {

  void* pool_alloc_memory(size_t size)
  {
    return TheServiceParticipant->pool_malloc(size);
  }

  void pool_free_memory(void* ptr)
  {
    TheServiceParticipant->pool_free(ptr);
  }

} }
