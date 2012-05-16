/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ShmemReceiveStrategy.h"
#include "ShmemDataLink.h"

#include "dds/DCPS/transport/framework/TransportHeader.h"

#include <cstring>

namespace OpenDDS {
namespace DCPS {

ShmemReceiveStrategy::ShmemReceiveStrategy(ShmemDataLink* link)
  : link_(link)
  , current_data_(0)
  , partial_recv_remaining_(0)
{
}

void
ShmemReceiveStrategy::read()
{
  if (partial_recv_remaining_) {
    VDBG((LM_DEBUG, "(%P|%t) ShmemReceiveStrategy::read link %@ "
          "resuming partial recv\n", link_));
    handle_dds_input(ACE_INVALID_HANDLE);
    return;
  }

  if (bound_name_.empty()) {
    bound_name_ = "Write-" + link_->local_address();
  }

  ShmemAllocator* alloc = link_->peer_allocator();
  void* mem;
  if (-1 == alloc->find(bound_name_.c_str(), mem)) {
    VDBG_LVL((LM_INFO, "(%P|%t) ShmemReceiveStrategy::read link %@ "
              "peer allocator not found, receive_bytes will close link\n",
              link_), 1);
    handle_dds_input(ACE_INVALID_HANDLE); // will return 0 to the TRecvStrateg.
    return;
  }

  if (!current_data_) {
    current_data_ = reinterpret_cast<ShmemData*>(mem);
  }

  for (ShmemData* start = 0; current_data_->status_ == SHMEM_DATA_FREE ||
         current_data_->status_ == SHMEM_DATA_RECV_DONE; ++current_data_) {
    if (!start) {
      start = current_data_;
    } else if (start == current_data_) {
      return; // none found => don't call handle_dds_input()
    }
    if (current_data_[1].status_ == SHMEM_DATA_END_OF_ALLOC) {
      current_data_ = reinterpret_cast<ShmemData*>(mem) - 1; // incremented by the for loop
    }
  }

  VDBG((LM_DEBUG, "(%P|%t) ShmemReceiveStrategy::read link %@ "
        "reading at control block #%d\n",
        link_, current_data_ - reinterpret_cast<ShmemData*>(mem)));
  // If we get this far, current_data_ points to the first SHMEM_DATA_IN_USE.
  // handle_dds_input() will call our receive_bytes() to get the data.
  handle_dds_input(ACE_INVALID_HANDLE);
}

ssize_t
ShmemReceiveStrategy::receive_bytes(iovec iov[],
                                    int n,
                                    ACE_INET_Addr& /*remote_address*/,
                                    ACE_HANDLE /*fd*/)
{
  VDBG((LM_DEBUG,
        "(%P|%t) ShmemReceiveStrategy::receive_bytes link %@\n", link_));

  // check that the writer's shared memory is still available
  ShmemAllocator* alloc = link_->peer_allocator();
  void* mem;
  if (-1 == alloc->find(bound_name_.c_str(), mem)
      || current_data_->status_ != SHMEM_DATA_IN_USE) {
    VDBG_LVL((LM_INFO,
              "(%P|%t) ShmemReceiveStrategy::receive_bytes link %@ closing\n",
              link_), 1);
    gracefully_disconnected_ = true; // do not attempt reconnect via relink()
    return 0; // close "connection"
  }

  ssize_t total = 0;

  const char* src_iter;
  char* dst_iter = 0;
  int i = 0; // current iovec index in iov[]
  size_t remaining;

  if (partial_recv_remaining_) {
    remaining = partial_recv_remaining_;
    src_iter = partial_recv_ptr_;
    // iov_base is void* on POSIX but char* on Win32, we'll have to cast:
    dst_iter = (char*)iov[0].iov_base;

  } else {
    remaining = TransportHeader::get_length(current_data_->transport_header_);
    const size_t hdr_sz = sizeof(current_data_->transport_header_);
    // BUFFER_LOW_WATER in the framework ensures a large enough buffer
    if (iov[0].iov_len < hdr_sz) {
      VDBG_LVL((LM_ERROR, "(%P|%t) ERROR: ShmemReceiveStrategy for link %@ "
                "receive buffer of length %d is too small\n",
                link_, iov[0].iov_len), 0);
      errno = ENOBUFS;
      return -1;
    }

    std::memcpy(iov[0].iov_base, current_data_->transport_header_, hdr_sz);
    total += hdr_sz;
    src_iter = current_data_->payload_;
    if (iov[0].iov_len > hdr_sz) {
      dst_iter = (char*)iov[0].iov_base + hdr_sz;
    } else if (n > 1) {
      dst_iter = (char*)iov[1].iov_base;
      i = 1;
    }
  }

  for (; i < n && remaining; ++i) {
    const size_t space = (i == 0) ? iov[i].iov_len - total : iov[i].iov_len,
      chunk = std::min(space, remaining);
    std::memcpy(dst_iter, src_iter, chunk);
    if (i < n - 1) {
      dst_iter = (char*)iov[i + 1].iov_base;
    }
    remaining -= chunk;
    total += chunk;
    src_iter += chunk;
  }

  if (remaining) {
    partial_recv_remaining_ = remaining;
    partial_recv_ptr_ = src_iter;
    VDBG((LM_DEBUG, "(%P|%t) ShmemReceiveStrategy for link %@ "
          "receive was partial\n", link_));
    link_->signal_semaphore();

  } else {
    partial_recv_remaining_ = 0;
    partial_recv_ptr_ = 0;
    VDBG((LM_DEBUG, "(%P|%t) ShmemReceiveStrategy for link %@ "
          "receive done\n", link_));
    current_data_->status_ = SHMEM_DATA_RECV_DONE;
  }

  return total;
}

void
ShmemReceiveStrategy::deliver_sample(ReceivedDataSample& sample,
                                     const ACE_INET_Addr& /*remote_address*/)
{
  switch (sample.header_.message_id_) {
  case SAMPLE_ACK:
    link_->ack_received(sample);
    break;

  case TRANSPORT_CONTROL:
    link_->control_received(sample);
    break;

  default:
    link_->data_received(sample);
  }
}

int
ShmemReceiveStrategy::start_i()
{
  return 0;
}

void
ShmemReceiveStrategy::stop_i()
{
}

} // namespace DCPS
} // namespace OpenDDS
