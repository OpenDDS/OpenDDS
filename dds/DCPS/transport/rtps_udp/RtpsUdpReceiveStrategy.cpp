/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpReceiveStrategy.h"
#include "RtpsUdpDataLink.h"

#include "ace/Reactor.h"

namespace OpenDDS {
namespace DCPS {

RtpsUdpReceiveStrategy::RtpsUdpReceiveStrategy(RtpsUdpDataLink* link)
  : link_(link)
  , last_received_()
{
}

ACE_HANDLE
RtpsUdpReceiveStrategy::get_handle() const
{
  return ACE_INVALID_HANDLE;
}

int
RtpsUdpReceiveStrategy::handle_input(ACE_HANDLE /*fd*/)
{
  return TransportReceiveStrategy::handle_input();  // delegate to parent
}

ssize_t
RtpsUdpReceiveStrategy::receive_bytes(iovec iov[],
                                      int n,
                                      ACE_INET_Addr& remote_address)
{
  return n;
}

void
RtpsUdpReceiveStrategy::deliver_sample(ReceivedDataSample& sample,
                                       const ACE_INET_Addr& remote_address)
{
  this->link_->data_received(sample);
}

int
RtpsUdpReceiveStrategy::start_i()
{
  ACE_Reactor* reactor = this->link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpReceiveStrategy::start_i: ")
                      ACE_TEXT("NULL reactor reference!\n")),
                     -1);
  }

  if (reactor->register_handler(this, ACE_Event_Handler::READ_MASK) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpReceiveStrategy::start_i: ")
                      ACE_TEXT("failed to register handler for DataLink!\n")),
                     -1);
  }

  this->enable_reassembly();
  return 0;
}

void
RtpsUdpReceiveStrategy::stop_i()
{
  ACE_Reactor* reactor = this->link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("RtpsUdpReceiveStrategy::stop_i: ")
               ACE_TEXT("NULL reactor reference!\n")));
    return;
  }

  reactor->remove_handler(this, ACE_Event_Handler::READ_MASK);
}

bool
RtpsUdpReceiveStrategy::check_header(const TransportHeader& header)
{
  return true;
}

} // namespace DCPS
} // namespace OpenDDS
