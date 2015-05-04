#ifndef dds_DCPS_PoolAllocationBase_h
#define dds_DCPS_PoolAllocationBase_h

#ifdef OPENDDS_SAFETY_PROFILE

#include "SafetyProfilePool.h"

namespace OpenDDS {
  namespace DCPS {

    class PoolAllocationBase
    {
    public:
      // TODO:  Exception behavior.
      void* operator new(size_t size) { return SafetyProfilePool::instance()->malloc(size); }
      void* operator new(size_t size, const std::nothrow_t&) { return SafetyProfilePool::instance()->malloc(size); }
    };
  }
}

#else

namespace OpenDDS {
namespace DCPS {

  class PoolAllocationBase { };

}
}

#endif

#endif /* dds_DCPS_PoolAllocationBase_h */
