/*
 * $Id$
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

namespace OpenDDS {
namespace DCPS {

ShmemSendStrategy::ShmemSendStrategy(ShmemDataLink* link)
  : TransportSendStrategy(TransportInst_rch(link->config(), false),
                          0,  // synch_resource
                          link->transport_priority(),
                          new NullSynchStrategy)
  , link_(link)
  , current_data_(0)
{
}

void
ShmemSendStrategy::start_i()
{
  bound_name_ = "Write-" + link_->local_address();
  ShmemAllocator* alloc = link_->local_allocator();
  const unsigned int n_bytes = 4096, //TODO: configurable
    n_elems = n_bytes / sizeof(ShmemData),
    extra = n_bytes % sizeof(ShmemData);
  void* mem = alloc->calloc(n_bytes);
  ShmemData* data = reinterpret_cast<ShmemData*>(mem);
  data[(extra >= sizeof(int)) ? n_elems : (n_elems - 1)].status_ =
    SHMEM_DATA_END_OF_ALLOC;
  alloc->bind(bound_name_.c_str(), mem);

  ShmemAllocator* peer = link_->peer_allocator();
  peer->find("Semaphore", mem);
  ShmemSharedSemaphore* sem = reinterpret_cast<ShmemSharedSemaphore*>(mem);
#ifdef ACE_WIN32
  HANDLE srcProc = ::OpenProcess(PROCESS_DUP_HANDLE, false /*bInheritHandle*/,
                                 link_->peer_pid());
  ::DuplicateHandle(srcProc, *sem, GetCurrentProcess(), &peer_semaphore_,
                    0 /*dwDesiredAccess -- ignored*/,
                    false /*bInheritHandle*/,
                    DUPLICATE_SAME_ACCESS /*dwOptions*/);
  ::CloseHandle(srcProc);
#else
  peer_semaphore_.sema_ = sem;
  peer_semaphore_.name_ = 0;
#endif
}

ssize_t
ShmemSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  //TODO: check that iov[0] is really the Transport Header
  //TODO: other error checks
  //TODO: use the ShmemTransport object to see if we already have the
  //      same payload data available in the pool (from other DataLinks),
  //      and if so, add a refcount to the start of the "from_pool" allocation

  size_t pool_alloc_size = 0;
  for (int i = 1 /* skip TransportHeader in [0] */; i < n; ++i) {
    pool_alloc_size += iov[i].iov_len;
  }

  ShmemAllocator* alloc = link_->local_allocator();
  void* from_pool = alloc->malloc(pool_alloc_size);
  char* payload = reinterpret_cast<char*>(from_pool);
  char* iter = payload;
  for (int i = 1 /* skip TransportHeader in [0] */; i < n; ++i) {
    std::memcpy(iter, iov[i].iov_base, iov[i].iov_len);
    iter += iov[i].iov_len;
  }

  void* mem;
  alloc->find(bound_name_.c_str(), mem);

  if (!current_data_) {
    current_data_ = reinterpret_cast<ShmemData*>(mem);
  }

  for (ShmemData* start = 0; current_data_->status_ == SHMEM_DATA_IN_USE;
       ++current_data_) {
    if (!start) {
      start = current_data_;
    } else if (start == current_data_) {
      return -1; // no space available
    } 
    if (current_data_[1].status_ == SHMEM_DATA_END_OF_ALLOC) {
      current_data_ = reinterpret_cast<ShmemData*>(mem) - 1; // incremented by the for loop
    }
  }

  if (current_data_->status_ == SHMEM_DATA_FREE) {
    std::memcpy(current_data_->transport_header_, iov[0].iov_base,
                std::min(sizeof(current_data_->transport_header_),
                         size_t(iov[0].iov_len)));
    current_data_->payload_ = payload;
    current_data_->status_ = SHMEM_DATA_IN_USE;
  }

  ACE_OS::sema_post(&peer_semaphore_);

  return pool_alloc_size + iov[0].iov_len;
}

void
ShmemSendStrategy::stop_i()
{
#ifdef ACE_WIN32
  ::CloseHandle(peer_semaphore_);
#endif
}

} // namespace DCPS
} // namespace OpenDDS
