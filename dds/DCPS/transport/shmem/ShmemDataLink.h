/*
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

#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/PeriodicTask.h>
#include <dds/DCPS/transport/framework/DataLink.h>

#include <ace/Local_Memory_Pool.h>
#include <ace/Malloc_T.h>
#include <ace/Pagefile_Memory_Pool.h>
#include <ace/PI_Malloc.h>
#include <ace/Process_Mutex.h>
#include <ace/Shared_Memory_Pool.h>

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

  int make_reservation(const GUID_t& remote_sub, const GUID_t& local_pub,
    const TransportSendListener_wrch& send_listener, bool reliable);

  int make_reservation(const GUID_t& remote_pub,
                       const GUID_t& local_sub,
                       const TransportReceiveListener_wrch& receive_listener,
                       bool reliable);

  void request_ack_received(ReceivedDataSample& sample);

  void control_received(ReceivedDataSample& sample);

  std::string local_address();
  std::string peer_address();
  pid_t peer_pid();

  ShmemAllocator* local_allocator();
  ShmemAllocator* peer_allocator();

  void read() { recv_strategy_->read(); }
  void signal_semaphore();
  ShmemTransport& impl() const;
  ShmemInst& config() const;

protected:
  ShmemSendStrategy_rch send_strategy_;
  ShmemReceiveStrategy_rch recv_strategy_;

  virtual void stop_i();

private:
  void send_association_msg(const GUID_t& local, const GUID_t& remote);
  void resend_association_msgs(const MonotonicTimePoint& now);

  std::string peer_address_;
  ShmemAllocator* peer_alloc_;
  ACE_Thread_Mutex peer_alloc_mutex_;
  ReactorTask_rch reactor_task_;

  struct GuidPair {
    const GUID_t local;
    const GUID_t remote;

    GuidPair(const GUID_t& local, const GUID_t& remote)
    : local(local)
    , remote(remote)
    {
    }

    bool operator<(const GuidPair& other) const
    {
      return GUID_tKeyLessThan()(local, other.local) && GUID_tKeyLessThan()(remote, other.remote);
    }
  };
  ACE_Thread_Mutex assoc_resends_mutex_;
  typedef std::map<GuidPair, size_t> AssocResends;
  AssocResends assoc_resends_;
  typedef PmfPeriodicTask<ShmemDataLink> SmPeriodicTask;
  DCPS::RcHandle<SmPeriodicTask> assoc_resends_task_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
# include "ShmemDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* OPENDDS_SHMEMDATALINK_H */
