/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_SHMEM_SHMEMALLOCATOR_H
#define OPENDDS_DCPS_TRANSPORT_SHMEM_SHMEMALLOCATOR_H

#include <dds/Versioned_Namespace.h>

#include <ace/Local_Memory_Pool.h>
#include <ace/Malloc_T.h>
#include <ace/Pagefile_Memory_Pool.h>
#include <ace/PI_Malloc.h>
#include <ace/Process_Mutex.h>
#include <ace/Shared_Memory_Pool.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ShmemInst;

#if defined ACE_WIN32 && !defined ACE_HAS_WINCE
#  define OPENDDS_SHMEM_WINDOWS
typedef ACE_Pagefile_Memory_Pool ShmemPool;
typedef HANDLE ShmemSharedSemaphore;

#elif !defined ACE_LACKS_SYSV_SHMEM \
      && defined ACE_HAS_POSIX_SEM \
      && !defined ACE_LACKS_UNNAMED_SEMAPHORE
#  define OPENDDS_SHMEM_UNIX
typedef ACE_Shared_Memory_Pool ShmemPool;
typedef sem_t ShmemSharedSemaphore;
#  if !defined ACE_HAS_POSIX_SEM_TIMEOUT && \
      !defined ACE_DISABLE_POSIX_SEM_TIMEOUT_EMULATION
#    define OPENDDS_SHMEM_UNIX_EMULATE_SEM_TIMEOUT
#  endif

// No Support for this Platform, Trying to Use Shared Memory Transport Will
// Yield a Runtime Error
#else
#  define OPENDDS_SHMEM_UNSUPPORTED
// These are just place holders
typedef ACE_Local_Memory_Pool ShmemPool;
typedef int ShmemSharedSemaphore;
#endif

typedef ACE_Malloc_T<ShmemPool, ACE_Process_Mutex, ACE_PI_Control_Block> ShmemAllocator;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_TRANSPORT_SHMEM_SHMEMALLOCATOR_H */
