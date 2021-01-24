/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_SHMEM_SHMEMDATALINK_H
#define OPENDDS_DCPS_TRANSPORT_SHMEM_SHMEMDATALINK_H

#include "Shmem_Export.h"

#include "ShmemSendStrategy.h"
#include "ShmemSendStrategy_rch.h"
#include "ShmemReceiveStrategy.h"
#include "ShmemReceiveStrategy_rch.h"

#include "dds/DCPS/transport/framework/DataLink.h"

#include "ace/Local_Memory_Pool.h"
#include "ace/Malloc_T.h"
#include "ace/Pagefile_Memory_Pool.h"
#include "ace/PI_Malloc.h"
#include "ace/Process_Mutex.h"
#include "ace/Shared_Memory_Pool.h"

#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ShmemInst;
class ShmemTransport;
class ReceivedDataSample;
typedef RcHandle<ShmemTransport> ShmemTransport_rch;

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

typedef ACE_Malloc_T<ShmemPool, ACE_Process_Mutex, ACE_PI_Control_Block>
  ShmemAllocator;

struct ShmemData {
  int status_;
  char transport_header_[TRANSPORT_HDR_SERIALIZED_SZ];
  ACE_Based_Pointer_Basic<char> payload_;
};

/**
 * values for ShmemData::status_
 */
enum {
  SHMEM_DATA_FREE = 0,
  SHMEM_DATA_IN_USE = 1,
  SHMEM_DATA_RECV_DONE = 2,
  SHMEM_DATA_END_OF_ALLOC = -1
};

class OpenDDS_Shmem_Export ShmemDataLink
  : public DataLink {
public:

  ShmemDataLink(ShmemTransport& transport);

  bool open(const std::string& peer_address);

  void control_received(ReceivedDataSample& sample);

  std::string local_address();
  std::string peer_address();
  pid_t peer_pid();

  ShmemAllocator* local_allocator();
  ShmemAllocator* peer_allocator();

  void read() { recv_strategy_->read(); }
  void signal_semaphore();
  ShmemTransport& impl() const;

protected:
  ShmemInst* config_;

  ShmemSendStrategy_rch send_strategy_;
  ShmemReceiveStrategy_rch recv_strategy_;

  virtual void stop_i();

private:
  std::string peer_address_;
  ShmemAllocator* peer_alloc_;
  ACE_Thread_Mutex mutex_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
# include "ShmemDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* OPENDDS_SHMEMDATALINK_H */
