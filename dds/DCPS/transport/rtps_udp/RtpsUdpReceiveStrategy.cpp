/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpReceiveStrategy.h"
#include "RtpsUdpDataLink.h"

#include "dds/DCPS/RTPS/BaseMessageTypes.h"

#include "ace/Reactor.h"

namespace OpenDDS {
namespace DCPS {

RtpsUdpReceiveStrategy::RtpsUdpReceiveStrategy(RtpsUdpDataLink* link)
  : link_(link)
  , last_received_()
  , receiver_(OpenDDS::DCPS::GUIDPREFIX_UNKNOWN) //TODO: get local prefix from link
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
RtpsUdpReceiveStrategy::check_header(const RtpsTransportHeader& header)
{
  return true;
}

bool
RtpsUdpReceiveStrategy::check_header(const RtpsSampleHeader& header)
{
  return true;
}

RtpsUdpReceiveStrategy::MessageReceiver::MessageReceiver(
  const OpenDDS::RTPS::GuidPrefix_t& local)
{
  assign(local_, local);
}

void
RtpsUdpReceiveStrategy::MessageReceiver::reset(const ACE_INET_Addr& addr,
                                               const OpenDDS::RTPS::Header& hdr)
{
  using namespace OpenDDS::RTPS;
  // see RTPS spec v2.1 section 8.3.4 table 8.16 and section 8.3.6.4
  source_version_ = hdr.version;
  source_vendor_ = hdr.vendorId;

  assign(source_guid_prefix_, hdr.guidPrefix);
  assign(dest_guid_prefix_, local_);

  unicast_reply_locator_list_.length(0);
  unicast_reply_locator_list_[0].kind =
    addr.get_type() == AF_INET6 ? LOCATOR_KIND_UDPv6 : LOCATOR_KIND_UDPv4;
  unicast_reply_locator_list_[0].port = LOCATOR_PORT_INVALID;
  assign(unicast_reply_locator_list_[0].address, addr);

  multicast_reply_locator_list_.length(0);
  multicast_reply_locator_list_[0].kind =
    addr.get_type() == AF_INET6 ? LOCATOR_KIND_UDPv6 : LOCATOR_KIND_UDPv4;
  multicast_reply_locator_list_[0].port = LOCATOR_PORT_INVALID;
  assign(multicast_reply_locator_list_[0].address, LOCATOR_ADDRESS_INVALID);

  have_timestamp_ = false;
  timestamp_ = TIME_INVALID;
}

void
RtpsUdpReceiveStrategy::MessageReceiver::submsg(
  const OpenDDS::RTPS::InfoDestinationSubmessage& id)
{
  // see RTPS spec v2.1 section 8.3.7.7.4
  for (size_t i = 0; i < sizeof(GuidPrefix_t); ++i) {
    if (id.guidPrefix[i]) { // if some byte is > 0, it's not UNKNOWN
      assign(dest_guid_prefix_, id.guidPrefix);
      return;
    }
  }
  assign(dest_guid_prefix_, local_);
}

void
RtpsUdpReceiveStrategy::MessageReceiver::submsg(
  const OpenDDS::RTPS::InfoReplySubmessage& ir)
{
  // see RTPS spec v2.1 section 8.3.7.8.4
  unicast_reply_locator_list_.length(ir.unicastLocatorList.length());
  for (CORBA::ULong i = 0; i < ir.unicastLocatorList.length(); ++i) {
    unicast_reply_locator_list_[i] = ir.unicastLocatorList[i];
  }

  if (ir.smHeader.flags & 2 /* MulticastFlag */) {
    multicast_reply_locator_list_.length(ir.multicastLocatorList.length());
    for (CORBA::ULong i = 0; i < ir.multicastLocatorList.length(); ++i) {
      multicast_reply_locator_list_[i] = ir.multicastLocatorList[i];
    }

  } else {
    multicast_reply_locator_list_.length(0);
  }
}

void
RtpsUdpReceiveStrategy::MessageReceiver::submsg(
  const OpenDDS::RTPS::InfoReplyIp4Submessage& iri4)
{
  // see RTPS spec v2.1 sections 8.3.7.8.4 and 9.4.5.14
  unicast_reply_locator_list_.length(1);
  unicast_reply_locator_list_[0].kind = OpenDDS::RTPS::LOCATOR_KIND_UDPv4;
  unicast_reply_locator_list_[0].port = iri4.unicastLocator.port;
  assign(unicast_reply_locator_list_[0].address, iri4.unicastLocator.address);

  if (iri4.smHeader.flags & 2 /* MulticastFlag */) {
    multicast_reply_locator_list_.length(1);
    multicast_reply_locator_list_[0].kind = OpenDDS::RTPS::LOCATOR_KIND_UDPv4;
    multicast_reply_locator_list_[0].port = iri4.multicastLocator.port;
    assign(multicast_reply_locator_list_[0].address,
           iri4.multicastLocator.address);
  } else {
    multicast_reply_locator_list_.length(0);
  }
}

void
RtpsUdpReceiveStrategy::MessageReceiver::submsg(
  const OpenDDS::RTPS::InfoTimestampSubmessage& it)
{
  // see RTPS spec v2.1 section 8.3.7.9.10
  if (!(it.smHeader.flags & 2 /* InvalidateFlag */)) {
    have_timestamp_ = true;
    timestamp_ = it.timestamp;
  } else {
    have_timestamp_ = false;
  }
}

void
RtpsUdpReceiveStrategy::MessageReceiver::submsg(
  const OpenDDS::RTPS::InfoSourceSubmessage& is)
{
  // see RTPS spec v2.1 section 8.3.7.9.4
  assign(source_guid_prefix_, is.guidPrefix);
  source_version_ = is.version;
  source_vendor_ = is.vendorId;
  unicast_reply_locator_list_.length(1);
  unicast_reply_locator_list_[0] = OpenDDS::RTPS::LOCATOR_INVALID;
  multicast_reply_locator_list_.length(1);
  multicast_reply_locator_list_[0] = OpenDDS::RTPS::LOCATOR_INVALID;
  have_timestamp_ = false;
}

} // namespace DCPS
} // namespace OpenDDS
