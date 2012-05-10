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

namespace OpenDDS {
namespace DCPS {

ShmemReceiveStrategy::ShmemReceiveStrategy(ShmemDataLink* link)
  : link_(link)
  , expected_(SequenceNumber::SEQUENCENUMBER_UNKNOWN())
  , current_data_(0)
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

  if (!current_data_) {
    current_data_ = reinterpret_cast<ShmemData*>(mem);
  }

  for (ShmemData* start = 0; current_data_->status_ == SHMEM_DATA_FREE;
       ++current_data_) {
    if (!start) {
      start = current_data_;
    } else if (start == current_data_) {
      return; // none found => don't call handle_dds_input()
    } else if (current_data_[1].status_ == SHMEM_DATA_END_OF_ALLOC) {
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
  if (-1 == alloc->find(bound_name_.c_str(), mem)) {
    return 0; // close "connection"
  }

  if (current_data_->status_ == SHMEM_DATA_IN_USE) {
    std::memcpy(iov[0].iov_base, current_data_->transport_header_,
                sizeof(current_data_->transport_header_));

    const u_long theader = sizeof(current_data_->transport_header_);
    const ACE_UINT32 size =
      TransportHeader::get_length(current_data_->transport_header_);
    u_long remaining = size,
      space = iov[0].iov_len - theader,
      chunk = std::min(space, remaining);

    std::memcpy(iov[0].iov_base + theader, current_data_->payload_, chunk);
    remaining -= chunk;

    char* iter = current_data_->payload_ + chunk;
    for (int i = 1; i < n && remaining; ++i) {
      chunk = std::min(iov[i].iov_len, u_long(size));
      std::memcpy(iov[i].iov_base, iter, chunk);
      remaining -= chunk;
    }

    alloc->free(current_data_->payload_); // this will eventually be refcounted
    current_data_->status_ = SHMEM_DATA_FREE;

    return theader + size;
  }

  return 0;
}

void
ShmemReceiveStrategy::deliver_sample(ReceivedDataSample& sample,
                                     const ACE_INET_Addr& remote_address)
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
