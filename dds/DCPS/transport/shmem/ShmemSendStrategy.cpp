/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ShmemSendStrategy.h"
#include "ShmemDataLink.h"
#include "ShmemInst.h"

#include "dds/DCPS/transport/framework/NullSynchStrategy.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ShmemSendStrategy::ShmemSendStrategy(ShmemDataLink* link)
  : TransportSendStrategy(0, link->impl(),
                          0,  // synch_resource
                          link->transport_priority(),
                          make_rch<NullSynchStrategy>())
  , link_(link)
  , current_data_(0)
  , datalink_control_size_(link->impl().config().datalink_control_size_)
{
#ifdef OPENDDS_SHMEM_UNIX
  memset(&peer_semaphore_, 0, sizeof(peer_semaphore_));
#endif
}

bool
ShmemSendStrategy::start_i()
{
  bound_name_ = "Write-" + link_->peer_address();
  ShmemAllocator* alloc = link_->local_allocator();

  const size_t n_elems = datalink_control_size_ / sizeof(ShmemData),
    extra = datalink_control_size_ % sizeof(ShmemData);

  void* mem = 0;
  if (alloc == 0 || (mem = alloc->calloc(datalink_control_size_)) == 0) {
    VDBG_LVL((LM_ERROR, "(%P|%t) ERROR: ShmemSendStrategy for link %@ failed "
              "to allocate %B bytes for control\n", link_, datalink_control_size_), 0);
    return false;
  }

  ShmemData* data = reinterpret_cast<ShmemData*>(mem);
  const size_t limit = (extra >= sizeof(int)) ? n_elems : (n_elems - 1);
  data[limit].status_ = SHMEM_DATA_END_OF_ALLOC;
  alloc->bind(bound_name_.c_str(), mem);

  ShmemAllocator* peer = link_->peer_allocator();
  peer->find("Semaphore", mem);
  ShmemSharedSemaphore* sem = reinterpret_cast<ShmemSharedSemaphore*>(mem);
#if defined OPENDDS_SHMEM_WINDOWS
  HANDLE srcProc = ::OpenProcess(PROCESS_DUP_HANDLE, false /*bInheritHandle*/,
                                 link_->peer_pid());
  ::DuplicateHandle(srcProc, *sem, GetCurrentProcess(), &peer_semaphore_,
                    0 /*dwDesiredAccess -- ignored*/,
                    false /*bInheritHandle*/,
                    DUPLICATE_SAME_ACCESS /*dwOptions*/);
  ::CloseHandle(srcProc);
#elif defined OPENDDS_SHMEM_UNIX
  peer_semaphore_.sema_ = sem;
  peer_semaphore_.name_ = 0;
#else
  ACE_UNUSED_ARG(sem);
#endif
  return true;
}

ssize_t
ShmemSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  const size_t hdr_sz = sizeof(current_data_->transport_header_);
  if (static_cast<size_t>(iov[0].iov_len) != hdr_sz) {
    VDBG_LVL((LM_ERROR, "(%P|%t) ERROR: ShmemSendStrategy for link %@ "
              "expecting iov[0] of size %B, got %B\n",
              link_, hdr_sz, iov[0].iov_len), 0);
    return -1;
  }
  // see TransportHeader::valid(), but this is for the marshaled form
  if (std::memcmp(&TransportHeader::DCPS_PROTOCOL[0], iov[0].iov_base,
                  sizeof(TransportHeader::DCPS_PROTOCOL)) != 0) {
    VDBG_LVL((LM_ERROR, "(%P|%t) ERROR: ShmemSendStrategy for link %@ "
              "expecting iov[0] to contain the transport header\n", link_), 0);
    return -1;
  }

  //FUTURE: use the ShmemTransport object to see if we already have the
  //        same payload data available in the pool (from other DataLinks),
  //        and if so, add a refcount to the start of the "from_pool" allocation

  size_t pool_alloc_size = 0;
  for (int i = 1 /* skip TransportHeader in [0] */; i < n; ++i) {
    pool_alloc_size += iov[i].iov_len;
  }

  ShmemAllocator* alloc = link_->local_allocator();
  void* from_pool = 0;
  if (alloc == 0 || (from_pool = alloc->malloc(pool_alloc_size)) == 0) {
    VDBG_LVL((LM_ERROR, "(%P|%t) ERROR: ShmemSendStrategy for link %@ failed "
              "to allocate %B bytes for data\n", link_, pool_alloc_size), 0);
    errno = ENOMEM;
    return -1;
  }

  char* payload = reinterpret_cast<char*>(from_pool);
  char* iter = payload;
  for (int i = 1 /* skip TransportHeader in [0] */; i < n; ++i) {
    std::memcpy(iter, iov[i].iov_base, iov[i].iov_len);
    iter += iov[i].iov_len;
  }

  void* mem = 0;
  if (-1 == alloc->find(bound_name_.c_str(), mem) || mem == 0) {
    VDBG_LVL((LM_ERROR, "(%P|%t) ERROR: ShmemSendStrategy for link %@ failed "
              "to find control segment with bound name %C\n", link_, bound_name_.c_str()), 0);
    errno = ENOENT;
    return -1;
  }

  for (ShmemData* iter = reinterpret_cast<ShmemData*>(mem);
       iter->status_ != SHMEM_DATA_END_OF_ALLOC; ++iter) {
    if (iter->status_ == SHMEM_DATA_RECV_DONE) {
      alloc->free(iter->payload_);
      // This will eventually be refcounted so instead of a free(), the previous
      // statement would decrement the refcount and check for 0 before free().
      // See the 'FUTURE' comment above.
      iter->status_ = SHMEM_DATA_FREE;
      VDBG_LVL((LM_DEBUG, "(%P|%t) ShmemSendStrategy for link %@ "
                "releasing control block #%d\n", link_,
                iter - reinterpret_cast<ShmemData*>(mem)), 5);
    }
  }

  if (!current_data_) {
    current_data_ = reinterpret_cast<ShmemData*>(mem);
  }

  for (ShmemData* start = 0; current_data_->status_ == SHMEM_DATA_IN_USE ||
         current_data_->status_ == SHMEM_DATA_RECV_DONE; ++current_data_) {
    if (!start) {
      start = current_data_;
    } else if (start == current_data_) {
      VDBG_LVL((LM_ERROR, "(%P|%t) ERROR: ShmemSendStrategy for link %@ out of "
                "space for control\n", link_), 0);
      return -1;
    }
    if (current_data_[1].status_ == SHMEM_DATA_END_OF_ALLOC) {
      current_data_ = reinterpret_cast<ShmemData*>(mem) - 1; // incremented by the for loop
    }
  }

  if (current_data_->status_ == SHMEM_DATA_FREE) {
    VDBG((LM_DEBUG, "(%P|%t) ShmemSendStrategy for link %@ "
          "writing at control block #%d header %@ payload %@ len %B\n",
          link_, current_data_ - reinterpret_cast<ShmemData*>(mem),
          current_data_->transport_header_, payload, pool_alloc_size));
    std::memcpy(current_data_->transport_header_, iov[0].iov_base,
                sizeof(current_data_->transport_header_));
    current_data_->payload_ = payload;
    current_data_->status_ = SHMEM_DATA_IN_USE;
  } else {
    VDBG_LVL((LM_ERROR, "(%P|%t) ERROR: ShmemSendStrategy for link %@ "
              "failed to find space for control\n", link_), 0);
    return -1;
  }

  ACE_OS::sema_post(&peer_semaphore_);

  return pool_alloc_size + iov[0].iov_len;
}

void
ShmemSendStrategy::stop_i()
{
#ifdef OPENDDS_SHMEM_WINDOWS
  ::CloseHandle(peer_semaphore_);
#endif
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
