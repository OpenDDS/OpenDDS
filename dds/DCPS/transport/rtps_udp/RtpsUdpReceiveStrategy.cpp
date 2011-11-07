/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpReceiveStrategy.h"
#include "RtpsUdpDataLink.h"
#include "RtpsUdpInst.h"
#include "RtpsUdpTransport.h"

#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/MessageTypes.h"

#include "ace/Reactor.h"


namespace OpenDDS {
namespace DCPS {

RtpsUdpReceiveStrategy::RtpsUdpReceiveStrategy(RtpsUdpDataLink* link)
  : link_(link)
  , last_received_()
  , receiver_(link->local_prefix())
{
}

int
RtpsUdpReceiveStrategy::handle_input(ACE_HANDLE fd)
{
  return handle_dds_input(fd);
}

ssize_t
RtpsUdpReceiveStrategy::receive_bytes(iovec iov[],
                                      int n,
                                      ACE_INET_Addr& remote_address,
                                      ACE_HANDLE fd)
{
  if (fd == link_->unicast_socket().get_handle()) {
    const ssize_t ret = link_->unicast_socket().recv(iov, n, remote_address);
    remote_address_ = remote_address;
    return ret;

  } else {
    const ssize_t ret = link_->multicast_socket().recv(iov, n, remote_address);
    remote_address_ = remote_address;
    return ret;
  }
}

void
RtpsUdpReceiveStrategy::deliver_sample(ReceivedDataSample& sample,
                                       const ACE_INET_Addr& /*remote_address*/)
{
  using namespace OpenDDS::RTPS;

  if (std::memcmp(receiver_.dest_guid_prefix_, link_->local_prefix(),
                  sizeof(GuidPrefix_t))) {
    // Not our message, we may be on multicast listening to all the others.
    return;
  }

  const RtpsSampleHeader& rsh = received_sample_header();
  const SubmessageKind kind = rsh.submessage_._d();

  switch (kind) {
  case INFO_SRC:
  case INFO_REPLY_IP4:
  case INFO_DST:
  case INFO_REPLY:
  case INFO_TS:
    // No-op: the INFO_* submessages only modify the state of the
    // MessageReceiver (see check_header()), they are not passed up to DCPS.
    break;

  case DATA:
    receiver_.fill_header(sample.header_);
    link_->received(rsh.submessage_.data_sm(), receiver_.source_guid_prefix_,
                    receiver_.dest_guid_prefix_);
    link_->data_received(sample);
    break;

  case GAP:
    link_->received(rsh.submessage_.gap_sm(), receiver_.source_guid_prefix_,
                    receiver_.dest_guid_prefix_);
    break;

  case HEARTBEAT:
    link_->received(rsh.submessage_.heartbeat_sm(),
                    receiver_.source_guid_prefix_, receiver_.dest_guid_prefix_);
    break;

  case ACKNACK:
    link_->received(rsh.submessage_.acknack_sm(), receiver_.source_guid_prefix_,
                    receiver_.dest_guid_prefix_);
    break;

  case HEARTBEAT_FRAG:
    link_->received(rsh.submessage_.hb_frag_sm(), receiver_.source_guid_prefix_,
                    receiver_.dest_guid_prefix_);
    break;

  /* no case DATA_FRAG: by the time deliver_sample() is called, reassemble()
     has successfully reassembled the fragments and we now have a DATA submsg
   */
  default:
    break;
  }
}

int
RtpsUdpReceiveStrategy::start_i()
{
  ACE_Reactor* reactor = link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpReceiveStrategy::start_i: ")
                      ACE_TEXT("NULL reactor reference!\n")),
                     -1);
  }

  if (reactor->register_handler(link_->unicast_socket().get_handle(), this,
                                ACE_Event_Handler::READ_MASK) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpReceiveStrategy::start_i: ")
                      ACE_TEXT("failed to register handler for unicast\n")),
                     -1);
  }

  if (link_->config()->use_multicast_) {
    if (reactor->register_handler(link_->multicast_socket().get_handle(), this,
                                  ACE_Event_Handler::READ_MASK) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("RtpsUdpReceiveStrategy::start_i: ")
                        ACE_TEXT("failed to register handler for multicast\n")),
                       -1);
    }
  }

  return 0;
}

void
RtpsUdpReceiveStrategy::stop_i()
{
  ACE_Reactor* reactor = link_->get_reactor();
  if (reactor == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: ")
               ACE_TEXT("RtpsUdpReceiveStrategy::stop_i: ")
               ACE_TEXT("NULL reactor reference!\n")));
    return;
  }

  reactor->remove_handler(link_->unicast_socket().get_handle(),
                          ACE_Event_Handler::READ_MASK);

  if (link_->config()->use_multicast_) {
    reactor->remove_handler(link_->multicast_socket().get_handle(),
                            ACE_Event_Handler::READ_MASK);
  }
}

bool
RtpsUdpReceiveStrategy::check_header(const RtpsTransportHeader& header)
{
  receiver_.reset(remote_address_, header.header_);
  return header.valid();
}

bool
RtpsUdpReceiveStrategy::check_header(const RtpsSampleHeader& header)
{
  receiver_.submsg(header.submessage_);

  // save fragmentation details for use in reassemble()
  if (header.valid() && header.submessage_._d() == RTPS::DATA_FRAG) {
    const RTPS::DataFragSubmessage& rtps = header.submessage_.data_frag_sm();
    frags_.first = rtps.fragmentStartingNum.value;
    frags_.second = frags_.first + (rtps.fragmentsInSubmessage - 1);
  }

  return header.valid();
}

bool
RtpsUdpReceiveStrategy::reassemble(ReceivedDataSample& data)
{
  using namespace OpenDDS::RTPS;
  receiver_.fill_header(data.header_); // set publication_id_.guidPrefix
  if (reassembly_.reassemble(frags_, data)) {

    // Reassembly was successful, replace DataFrag with Data.  This doesn't have
    // to be a fully-formed DataSubmessage, just enough for this class to use 
    // in deliver_sample() which ends up calling RtpsUdpDataLink::received().
    // In particular we will need the SequenceNumber, but ignore the iQoS.

    RtpsSampleHeader& rsh = received_sample_header();
    const DataFragSubmessage& dfsm = rsh.submessage_.data_frag_sm();

    const CORBA::Octet data_flags = (data.header_.byte_order_ ? 1 : 0)
                                  | (data.header_.key_fields_only_ ? 8 : 4);
    const DataSubmessage dsm = {
      {DATA, data_flags, 0}, 0, DATA_OCTETS_TO_IQOS,
      dfsm.readerId, dfsm.writerId, dfsm.writerSN, ParameterList()};
    rsh.submessage_.data_sm(dsm);
    return true;
  }
  return false;
}

void
RtpsUdpReceiveStrategy::remove_frags_from_bitmap(CORBA::Long bitmap[],
                                                 CORBA::ULong num_bits,
                                                 const SequenceNumber& base,
                                                 const RepoId& pub_id)
{
  for (CORBA::ULong i = 0, x = 0, bit = 0; i < num_bits; ++i, ++bit) {
    if (bit == 32) bit = 0;

    if (bit == 0) {
      x = static_cast<CORBA::ULong>(bitmap[i / 32]);
      if (x == 0) {
        // skip an entire Long if it's all 0's (adds 32 due to ++i)
        i += 31;
        bit = 31;
        //FUTURE: this could be generalized with something like the x86 "bsr"
        //        instruction using compiler intrinsics, VC++ _BitScanReverse()
        //        and GCC __builtin_clz()
        continue;
      }
    }

    const CORBA::ULong mask = 1 << (31 - bit);
    if (x & mask) {
      if (reassembly_.has_frags(base + i, pub_id)) {
        x &= ~mask;
        bitmap[i / 32] = x;
      }
    }
  }
}

void
RtpsUdpReceiveStrategy::remove_fragments(const SequenceRange& range,
                                         const RepoId& pub_id)
{
  for (SequenceNumber sn = range.first; sn <= range.second; ++sn) {
    reassembly_.data_unavailable(sn, pub_id);
  }
}

bool
RtpsUdpReceiveStrategy::has_fragments(const SequenceRange& range,
                                      const RepoId& pub_id,
                                      FragmentInfo* frag_info)
{
  for (SequenceNumber sn = range.first; sn <= range.second; ++sn) {
    if (reassembly_.has_frags(sn, pub_id)) {
      if (frag_info) {
        std::pair<SequenceNumber, RTPS::FragmentNumberSet> p;
        p.first = sn;
        frag_info->push_back(p);
        RTPS::FragmentNumberSet& missing_frags = frag_info->back().second;
        missing_frags.bitmap.length(8); // start at max length
        missing_frags.bitmapBase.value =
          reassembly_.get_gaps(sn, pub_id, missing_frags.bitmap.get_buffer(),
                               8, missing_frags.numBits);
        // reduce length in case get_gaps() didn't need all that room
        missing_frags.bitmap.length((missing_frags.numBits + 31) / 32);
      } else {
        return true;
      }
    }
  }
  return frag_info ? !frag_info->empty() : false;
}


// MessageReceiver nested class

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

  unicast_reply_locator_list_.length(1);
  unicast_reply_locator_list_[0].kind =
    addr.get_type() == AF_INET6 ? LOCATOR_KIND_UDPv6 : LOCATOR_KIND_UDPv4;
  unicast_reply_locator_list_[0].port = LOCATOR_PORT_INVALID;
  RtpsUdpTransport::address_to_bytes(unicast_reply_locator_list_[0].address,
                                     addr);

  multicast_reply_locator_list_.length(1);
  multicast_reply_locator_list_[0].kind =
    addr.get_type() == AF_INET6 ? LOCATOR_KIND_UDPv6 : LOCATOR_KIND_UDPv4;
  multicast_reply_locator_list_[0].port = LOCATOR_PORT_INVALID;
  assign(multicast_reply_locator_list_[0].address, LOCATOR_ADDRESS_INVALID);

  have_timestamp_ = false;
  timestamp_ = TIME_INVALID;
}

void
RtpsUdpReceiveStrategy::MessageReceiver::submsg(
  const OpenDDS::RTPS::Submessage& s)
{
  using namespace OpenDDS::RTPS;

  switch (s._d()) {
  case INFO_TS:
    submsg(s.info_ts_sm());
    break;

  case INFO_SRC:
    submsg(s.info_src_sm());
    break;

  case INFO_REPLY_IP4:
    submsg(s.info_reply_ipv4_sm());
    break;

  case INFO_DST:
    submsg(s.info_dst_sm());
    break;

  case INFO_REPLY:
    submsg(s.info_reply_sm());
    break;

  default:
    break;
  }
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

void
RtpsUdpReceiveStrategy::MessageReceiver::fill_header(
  DataSampleHeader& header) const
{
  using namespace OpenDDS::RTPS;
  if (have_timestamp_) {
    header.source_timestamp_sec_ = timestamp_.seconds;
    header.source_timestamp_nanosec_ =
      static_cast<ACE_UINT32>(timestamp_.fraction / NANOS_TO_RTPS_FRACS + .5);
  }
  assign(header.publication_id_.guidPrefix, source_guid_prefix_);
}

} // namespace DCPS
} // namespace OpenDDS
