/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ShmemReceiveStrategy.h"
#include "ShmemDataLink.h"

namespace OpenDDS {
namespace DCPS {

ShmemReceiveStrategy::ShmemReceiveStrategy(ShmemDataLink* link)
  : link_(link)
  , expected_(SequenceNumber::SEQUENCENUMBER_UNKNOWN())
{
}

ssize_t
ShmemReceiveStrategy::receive_bytes(iovec iov[],
                                    int n,
                                    ACE_INET_Addr& remote_address,
                                    ACE_HANDLE /*fd*/)
{
  return 0;
}

void
ShmemReceiveStrategy::deliver_sample(ReceivedDataSample& sample,
                                     const ACE_INET_Addr& remote_address)
{
  switch (sample.header_.message_id_) {
  case SAMPLE_ACK:
    this->link_->ack_received(sample);
    break;

  case TRANSPORT_CONTROL:
    this->link_->control_received(sample);
    break;

  default:
    this->link_->data_received(sample);
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

bool
ShmemReceiveStrategy::check_header(const TransportHeader& header)
{
  return true;
}


} // namespace DCPS
} // namespace OpenDDS
