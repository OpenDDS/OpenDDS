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
  if (bound_name_.empty()) {
    bound_name_ = "Write-" + link_->peer_address();
  }

  ShmemAllocator* alloc = link_->peer_allocator();
  void* mem;
  if (-1 == alloc->find(bound_name_.c_str(), mem)) {
    handle_dds_input(ACE_INVALID_HANDLE); // will return 0 to the TRecvStrateg.
    return;
  }

  if (partial_recv_remaining_) {
    handle_dds_input(ACE_INVALID_HANDLE);
    return;
  }

  if (!current_data_) {
    current_data_ = reinterpret_cast<ShmemData*>(mem);
  }

  for (ShmemData* start = 0; current_data_->status_ == SHMEM_DATA_FREE;
       ++current_data_) {
    if (!start) {
      start = current_data_;
    } else if (start == current_data_) {
      return; // none found => don't call handle_dds_input()
    }
    if (current_data_[1].status_ == SHMEM_DATA_END_OF_ALLOC) {
      current_data_ = reinterpret_cast<ShmemData*>(mem) - 1; // incremented by the for loop
    }
  }

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
  ACE_DEBUG((LM_DEBUG, "ShmemReceiveStrategy::receive_bytes\n"));

  // check that the writer's shared memory is still available
  ShmemAllocator* alloc = link_->peer_allocator();
  void* mem;
  if (-1 == alloc->find(bound_name_.c_str(), mem)
      || current_data_->status_ != SHMEM_DATA_IN_USE) {
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
    //TODO: assuming iov[0] has room for the transport header
    std::memcpy(iov[0].iov_base, current_data_->transport_header_, hdr_sz);
    total += hdr_sz;
    src_iter = current_data_->payload_;
    if (hdr_sz < iov[0].iov_len) {
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
    link_->signal_semaphore();

  } else {
    alloc->free(current_data_->payload_); // this will eventually be refcounted
    current_data_->status_ = SHMEM_DATA_FREE;
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
