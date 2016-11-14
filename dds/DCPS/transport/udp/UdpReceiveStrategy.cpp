/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpReceiveStrategy.h"
#include "UdpDataLink.h"

#include "ace/Reactor.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

UdpReceiveStrategy::UdpReceiveStrategy(UdpDataLink* link)
  : link_(link)
  , expected_(SequenceNumber::SEQUENCENUMBER_UNKNOWN())
{
}

ACE_HANDLE
UdpReceiveStrategy::get_handle() const
{
  ACE_SOCK_Dgram& socket = this->link_->socket();
  return socket.get_handle();
}

int
UdpReceiveStrategy::handle_input(ACE_HANDLE fd)
{
  return this->handle_dds_input(fd);
}

ssize_t
UdpReceiveStrategy::receive_bytes(iovec iov[],
                                  int n,
                                  ACE_INET_Addr& remote_address,
                                  ACE_HANDLE /*fd*/)
{
  const ssize_t ret = this->link_->socket().recv(iov, n, remote_address);
  remote_address_ = remote_address;
  return ret;
}

void
UdpReceiveStrategy::deliver_sample(ReceivedDataSample& sample,
                                   const ACE_INET_Addr& remote_address)
{
  switch (sample.header_.message_id_) {

  case TRANSPORT_CONTROL:
    this->link_->control_received(sample, remote_address);
    break;

  default:
    this->link_->data_received(sample);
    break;
  }
}

int
UdpReceiveStrategy::start_i()
{
  ACE_Reactor* reactor = this->link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpReceiveStrategy::start_i: ")
                      ACE_TEXT("NULL reactor reference!\n")),
                     -1);
  }

  if (reactor->register_handler(this, ACE_Event_Handler::READ_MASK) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpReceiveStrategy::start_i: ")
                      ACE_TEXT("failed to register handler for DataLink!\n")),
                     -1);
  }

  if (Transport_debug_level > 5) {
    ACE_INET_Addr addr;
    link_->socket().get_local_addr(addr);
    ACE_DEBUG((LM_DEBUG, "(%P|%t) UdpReceiveStrategy::start_i: "
               "listening on %C:%hu\n",
               addr.get_host_addr(), addr.get_port_number()));
  }
  return 0;
}

void
UdpReceiveStrategy::stop_i()
{
  ACE_Reactor* reactor = this->link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("UdpReceiveStrategy::stop_i: ")
               ACE_TEXT("NULL reactor reference!\n")));
    return;
  }

  reactor->remove_handler(this, ACE_Event_Handler::READ_MASK);
}

bool
UdpReceiveStrategy::check_header(const TransportHeader& header)
{
  ReassemblyInfo& info = reassembly_[remote_address_];

  if (header.sequence_ != info.second &&
      expected_ != SequenceNumber::SEQUENCENUMBER_UNKNOWN()) {
    VDBG_LVL((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: UdpReceiveStrategy::check_header ")
               ACE_TEXT("expected %q received %q\n"),
               info.second.getValue(), header.sequence_.getValue()), 2);
    SequenceRange range(info.second, header.sequence_.previous());
    info.first.data_unavailable(range);
  }

  info.second = header.sequence_;
  ++info.second;
  return true;
}

bool
UdpReceiveStrategy::reassemble(ReceivedDataSample& data)
{
  ReassemblyInfo& info = reassembly_[remote_address_];
  const TransportHeader& header = received_header();
  return info.first.reassemble(header.sequence_, header.first_fragment_, data);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
