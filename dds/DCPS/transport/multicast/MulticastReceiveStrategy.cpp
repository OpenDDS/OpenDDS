/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastReceiveStrategy.h"
#include "MulticastDataLink.h"

#include "ace/Reactor.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

MulticastReceiveStrategy::MulticastReceiveStrategy(MulticastDataLink* link)
  : link_(link)
{
}

ACE_HANDLE
MulticastReceiveStrategy::get_handle() const
{
  ACE_SOCK_Dgram_Mcast& socket = this->link_->socket();
  return socket.get_handle();
}

int
MulticastReceiveStrategy::handle_input(ACE_HANDLE fd)
{
  const int result = this->handle_dds_input(fd);
  if (result >= 0 && this->pdu_remaining()) {
    VDBG_LVL((LM_DEBUG, "(%P|%t) MulticastReceiveStrategy[%@]::handle_input "
      "resetting with %B bytes remaining\n", this, this->pdu_remaining()), 4);
    this->reset();
  }
  return result;
}

ssize_t
MulticastReceiveStrategy::receive_bytes(iovec iov[],
                                        int n,
                                        ACE_INET_Addr& remote_address,
                                        ACE_HANDLE /*fd*/)
{
  ACE_SOCK_Dgram_Mcast& socket = this->link_->socket();
  return socket.recv(iov, n, remote_address);
}

bool
MulticastReceiveStrategy::check_header(const TransportHeader& header)
{
  return this->link_->check_header(header);
}

bool
MulticastReceiveStrategy::check_header(const DataSampleHeader& header)
{
  return this->link_->check_header(header);
}

void
MulticastReceiveStrategy::deliver_sample(ReceivedDataSample& sample,
                                         const ACE_INET_Addr& /*remote_address*/)
{
  this->link_->sample_received(sample);
}

bool
MulticastReceiveStrategy::reassemble(ReceivedDataSample& data)
{
  return this->link_->reassemble(data, received_header());
}

int
MulticastReceiveStrategy::start_i()
{
  ACE_Reactor* reactor = this->link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastReceiveStrategy::start_i: ")
                      ACE_TEXT("NULL reactor reference!\n")),
                     -1);
  }

  if (reactor->register_handler(this, ACE_Event_Handler::READ_MASK) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("MulticastReceiveStrategy::start_i: ")
                      ACE_TEXT("failed to register handler for DataLink!\n")),
                     -1);
  }

  return 0;
}

void
MulticastReceiveStrategy::stop_i()
{
  ACE_Reactor* reactor = this->link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("MulticastReceiveStrategy::stop_i: ")
               ACE_TEXT("NULL reactor reference!\n")));
    return;
  }

  reactor->remove_handler(this, ACE_Event_Handler::READ_MASK);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
