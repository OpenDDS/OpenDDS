/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpDataLink.h"
#include "RtpsUdpTransport.h"
#include "RtpsUdpInst.h"

#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/transport/framework/TransportCustomizedElement.h"
#include "dds/DCPS/transport/framework/TransportSendElement.h"
#include "dds/DCPS/transport/framework/TransportSendControlElement.h"

#include "dds/DCPS/RTPS/RtpsMessageTypesTypeSupportImpl.h"
#include "dds/DCPS/RTPS/MessageTypes.h"

#include "ace/Default_Constants.h"
#include "ace/Log_Msg.h"
#include "ace/Message_Block.h"
#include "ace/Reactor.h"

#include <cstring>

#ifndef __ACE_INLINE__
# include "RtpsUdpDataLink.inl"
#endif  /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

RtpsUdpDataLink::RtpsUdpDataLink(RtpsUdpTransport* transport,
                                 const GuidPrefix_t& local_prefix,
                                 RtpsUdpInst* config,
                                 TransportReactorTask* reactor_task)
  : DataLink(transport, // 3 data link "attributes", below, are unused
             0,         // priority
             false,     // is_loopback
             false),    // is_active
    config_(config),
    reactor_task_(reactor_task, false),
    transport_customized_element_allocator_(40,
                                            sizeof(TransportCustomizedElement)),
    multi_buff_(this, config->nak_depth_),
    nack_reply_(this, &RtpsUdpDataLink::send_nack_replies,
                config->nak_response_delay_),
    heartbeat_reply_(this, &RtpsUdpDataLink::send_heartbeat_replies,
                     config->heartbeat_response_delay_),
    heartbeat_(this)
{
  std::memcpy(local_prefix_, local_prefix, sizeof(GuidPrefix_t));
}

bool
RtpsUdpDataLink::open()
{
  if (unicast_socket_.open(config_->local_address_) != 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpDataLink::open: socket open: %m\n")),
                     false);
  }

  if (config_->use_multicast_) {
    if (multicast_socket_.join(config_->multicast_group_address_) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("RtpsUdpDataLink::open: ")
                        ACE_TEXT("ACE_SOCK_Dgram_Mcast::join failed: %m\n")),
                       false);
    }
  }

  send_strategy_->send_buffer(&multi_buff_);

  // Set up info_dst_ and info_reply_ messages for use with ACKNACKS,
  // these won't change during the life of the datalink.
  using namespace OpenDDS::RTPS;
  info_dst_.smHeader.submessageId = INFO_DST;
  info_dst_.smHeader.flags = 1 /*FLAG_E*/;
  info_dst_.smHeader.submessageLength = INFO_DST_SZ;
  std::memcpy(info_dst_.guidPrefix, local_prefix_, sizeof(GuidPrefix_t));

  info_reply_.smHeader.submessageId = INFO_REPLY;
  info_reply_.smHeader.flags = 1 /*FLAG_E*/;
  info_reply_.unicastLocatorList.length(1);
  info_reply_.unicastLocatorList[0].kind =
    (config_->local_address_.get_type() == AF_INET6)
    ? LOCATOR_KIND_UDPv6 : LOCATOR_KIND_UDPv4;
  info_reply_.unicastLocatorList[0].port =
    config_->local_address_.get_port_number();
  RtpsUdpTransport::address_to_bytes(info_reply_.unicastLocatorList[0].address,
                                     config_->local_address_);
  if (config_->use_multicast_) {
    info_reply_.multicastLocatorList.length(1);
    info_reply_.multicastLocatorList[0].kind =
      (config_->multicast_group_address_.get_type() == AF_INET6)
      ? LOCATOR_KIND_UDPv6 : LOCATOR_KIND_UDPv4;
    info_reply_.multicastLocatorList[0].port =
      config_->multicast_group_address_.get_port_number();
    RtpsUdpTransport::address_to_bytes(
      info_reply_.multicastLocatorList[0].address,
      config_->multicast_group_address_);
  }

  size_t size = 0, padding = 0;
  gen_find_size(info_reply_, size, padding);
  info_reply_.smHeader.submessageLength =
    static_cast<CORBA::UShort>(size + padding) - SMHDR_SZ;

  if (start(static_rchandle_cast<TransportSendStrategy>(send_strategy_),
            static_rchandle_cast<TransportStrategy>(recv_strategy_)) != 0) {
    stop_i();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpDataLink::open: start failed!\n")),
                     false);
  }

  return true;
}

void
RtpsUdpDataLink::add_locator(const RepoId& remote_id,
                             const ACE_INET_Addr& address)
{
  locators_[remote_id] = address;
}

void
RtpsUdpDataLink::get_locators(const RepoId& local_id,
                              std::set<ACE_INET_Addr>& addrs) const
{
  using std::map;
  typedef map<RepoId, ACE_INET_Addr, GUID_tKeyLessThan>::const_iterator iter_t;

  if (local_id == GUID_UNKNOWN) {
    for (iter_t iter = locators_.begin(); iter != locators_.end(); ++iter) {
      addrs.insert(iter->second);
    }
    return;
  }

  const GUIDSeq_var peers = peer_ids(local_id);
  if (!peers.ptr()) {
    return;
  }
  for (CORBA::ULong i = 0; i < peers->length(); ++i) {
    const iter_t iter = locators_.find(peers[i]);
    if (iter == locators_.end()) {
      // log error...
    } else {
      addrs.insert(iter->second);
    }
  }
}

void
RtpsUdpDataLink::associated(const RepoId& local_id, const RepoId& remote_id,
                            bool reliable)
{
  if (!reliable) {
    return;
  }

  GuidConverter conv(local_id);
  EntityKind kind = conv.entityKind();
  if (kind == KIND_WRITER) {
    writers_[local_id].remote_readers_[remote_id];
    heartbeat_.enable();

  } else if (kind == KIND_READER) {
    RtpsReaderMap::iterator rr = readers_.find(local_id);
    if (rr == readers_.end()) {
      rr = readers_.insert(RtpsReaderMap::value_type(local_id, RtpsReader()))
        .first;
    }
    rr->second.remote_writers_[remote_id];
    reader_index_.insert(RtpsReaderIndex::value_type(remote_id, rr));
  }
}

void
RtpsUdpDataLink::release_reservations_i(const RepoId& remote_id,
                                        const RepoId& local_id)
{
  using std::pair;
  GuidConverter conv(local_id);
  EntityKind kind = conv.entityKind();
  if (kind == KIND_WRITER) {
    RtpsWriterMap::iterator rw = writers_.find(local_id);

    if (rw != writers_.end()) {
      rw->second.remote_readers_.erase(remote_id);

      if (rw->second.remote_readers_.empty()) {
        writers_.erase(rw);

        if (writers_.empty()) {
          heartbeat_.disable();
        }
      }
    }

  } else if (kind == KIND_READER) {
    RtpsReaderMap::iterator rr = readers_.find(local_id);

    if (rr != readers_.end()) {
      rr->second.remote_writers_.erase(remote_id);

      for (pair<RtpsReaderIndex::iterator, RtpsReaderIndex::iterator> iters =
             reader_index_.equal_range(remote_id);
           iters.first != iters.second;) {
        if (iters.first->second == rr) {
          reader_index_.erase(iters.first++);
        } else {
          ++iters.first;
        }
      }

      if (rr->second.remote_writers_.empty()) {
        readers_.erase(rr);
      }
    }
  }
}

void
RtpsUdpDataLink::stop_i()
{
  nack_reply_.cancel();
  heartbeat_reply_.cancel();
  heartbeat_.disable();
}

void
RtpsUdpDataLink::MultiSendBuffer::retain_all(RepoId pub_id)
{
  RtpsWriterMap::iterator wi = outer_->writers_.find(pub_id);
  if (wi != outer_->writers_.end() && !wi->second.send_buff_.is_nil()) {
    wi->second.send_buff_->retain_all(pub_id);
  }
}

void
RtpsUdpDataLink::MultiSendBuffer::insert(SequenceNumber /*transport_seq*/,
                                         TransportSendStrategy::QueueType* q,
                                         ACE_Message_Block* chain)
{
  const SequenceNumber seq = q->peek()->sequence();
  if (seq == SequenceNumber::SEQUENCENUMBER_UNKNOWN()) {
    return;
  }

  const RepoId pub_id = q->peek()->publication_id();

  RtpsWriterMap::iterator wi = outer_->writers_.find(pub_id);
  if (wi == outer_->writers_.end()) {
    return; // this datawriter is not reliable
  }

  RcHandle<SingleSendBuffer>& send_buff = wi->second.send_buff_;

  if (send_buff.is_nil()) {
    send_buff = new SingleSendBuffer(outer_->config_->nak_depth_, 1 /*mspp*/);
    send_buff->bind(outer_->send_strategy_.in());
  }

  send_buff->insert(seq, q, chain);
}

TransportQueueElement*
RtpsUdpDataLink::customize_queue_element(TransportQueueElement* element)
{
  const ACE_Message_Block* msg = element->msg();
  if (!msg) {
    return element;
  }

  OpenDDS::RTPS::SubmessageSeq subm;
  add_gap_submsg(subm, *element);

  TransportSendElement* tse = dynamic_cast<TransportSendElement*>(element);
  TransportCustomizedElement* tce =
    dynamic_cast<TransportCustomizedElement*>(element);
  TransportSendControlElement* tsce =
    dynamic_cast<TransportSendControlElement*>(element);

  ACE_Message_Block* data = 0;

  // Based on the type of 'element', find and duplicate the data payload
  // continuation block.
  if (tsce) {        // Control message
    data = msg->cont()->duplicate();
    // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader
    RtpsSampleHeader::populate_data_control_submessages(
              subm, *tsce, this->requires_inline_qos(tsce->publication_id()));

  } else if (tse) {  // Basic data message
    // {DataSampleHeader} -> {Data Payload}
    data = msg->cont()->duplicate();
    const DataSampleListElement* dsle = tse->sample();
    // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader
    RtpsSampleHeader::populate_data_sample_submessages(
              subm, *dsle, this->requires_inline_qos(dsle->publication_id_));

  } else if (tce) {  // Customized data message
    // {DataSampleHeader} -> {Content Filtering GUIDs} -> {Data Payload}
    data = msg->cont()->cont()->duplicate();
    const DataSampleListElement* dsle = tce->original_send_element()->sample();
    // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader
    RtpsSampleHeader::populate_data_sample_submessages(
              subm, *dsle, this->requires_inline_qos(dsle->publication_id_));

  } else {
    //TODO: handle other types?
    return element;
  }

  size_t size = 0, padding = 0;
  for (CORBA::ULong i = 0; i < subm.length(); ++i) {
    if ((size + padding) % 4) {
      padding += 4 - ((size + padding) % 4);
    }
    gen_find_size(subm[i], size, padding);
  }

  ACE_Message_Block* hdr =
    new ACE_Message_Block(size + padding, ACE_Message_Block::MB_DATA, data);

  for (CORBA::ULong i = 0; i < subm.length(); ++i) {
    // byte swapping is handled in the operator<<() implementation
    Serializer ser(hdr, false, Serializer::ALIGN_CDR);
    ser << subm[i];
    const size_t len = hdr->length();
    if (len % 4) {
      hdr->wr_ptr(4 - (len % 4));
    }
  }

  // TODO: Decide what to do with this allocator.  Currently, we use a locally
  // created allocator.  Other options, pass a null reference (use the heap) or
  // somehow get the allocator from the WriteDataContainer.
  TransportCustomizedElement* rtps =
    TransportCustomizedElement::alloc(element, false,
                                      &this->transport_customized_element_allocator_);
  rtps->set_msg(hdr);

  // Let the framework know each TransportCustomizedElement must be in its own
  // Transport packet (i.e. have its own RTPS Message Header).
  rtps->set_requires_exclusive();
  return rtps;
}

bool
RtpsUdpDataLink::requires_inline_qos(const PublicationId& /*pub_id*/)
{
  if (this->force_inline_qos_) {
    // Force true for testing purposes
    return true;
  } else {
    // TODO: replace this with logic from reader based on discovery
    return false;
  }
}

bool RtpsUdpDataLink::force_inline_qos_ = false;

void
RtpsUdpDataLink::add_gap_submsg(OpenDDS::RTPS::SubmessageSeq& msg,
                                const TransportQueueElement& tqe)
{
  // These are the GAP submessages that we'll send directly in-line with the
  // DATA when we notice that the DataWriter has deliberately skipped seq #s.
  // There are other GAP submessages generated in response to reader ACKNACKS,
  // see send_nack_replies().
  using namespace OpenDDS::RTPS;

  const SequenceNumber seq = tqe.sequence();
  const RepoId pub = tqe.publication_id();
  if (seq == SequenceNumber::SEQUENCENUMBER_UNKNOWN() || pub == GUID_UNKNOWN) {
    return;
  }

  const RtpsWriterMap::iterator wi = writers_.find(pub);
  if (wi == writers_.end()) {
    return; // not a reliable writer, does not send GAPs
  }

  RtpsWriter& rw = wi->second;

  if (seq != rw.expected_) {
    SequenceNumber firstMissing = rw.expected_;

    // RTPS v2.1 8.3.7.4: the Gap sequence numbers are those in the range
    // [gapStart, gapListBase) and those in the SNSet.
    const SequenceNumber_t gapStart = {firstMissing.getHigh(),
                                       firstMissing.getLow()},
                           gapListBase = {seq.getHigh(),
                                          seq.getLow()};

    // We are not going to enable any bits in the "bitmap" of the SNSet,
    // but the "numBits" and the bitmap.length must both be > 0.
    LongSeq8 bitmap;
    bitmap.length(1);
    bitmap[0] = 0;

    GapSubmessage gap = {
      {GAP, 1 /*FLAG_E*/, 0 /*length determined below*/},
      ENTITYID_UNKNOWN, // readerId: applies to all matched readers
      pub.entityId,
      gapStart,
      {gapListBase, 1, bitmap}
    };

    size_t size = 0, padding = 0;
    gen_find_size(gap, size, padding);
    gap.smHeader.submessageLength =
      static_cast<CORBA::UShort>(size + padding) - SMHDR_SZ;

    const CORBA::ULong i = msg.length();
    msg.length(i + 1);
    msg[i].gap_sm(gap);
  }

  rw.expected_ = seq;
  ++rw.expected_;
}

void
RtpsUdpDataLink::received(const OpenDDS::RTPS::DataSubmessage& data,
                          const GuidPrefix_t& src_prefix,
                          const GuidPrefix_t& dst_prefix)
{
  datareader_dispatch(data, src_prefix, dst_prefix,
                      &RtpsUdpDataLink::process_data_i);
}

void
RtpsUdpDataLink::process_data_i(const OpenDDS::RTPS::DataSubmessage& data,
                                const RepoId& src,
                                RtpsReaderMap::value_type& rr)
{
  WriterInfoMap::iterator wi = rr.second.remote_writers_.find(src);
  if (wi != rr.second.remote_writers_.end()) {
    SequenceNumber seq;
    seq.setValue(data.writerSN.high, data.writerSN.low);
    wi->second.recvd_.insert(seq);
  }
}

void
RtpsUdpDataLink::received(const OpenDDS::RTPS::GapSubmessage& gap,
                          const GuidPrefix_t& src_prefix,
                          const GuidPrefix_t& dst_prefix)
{
  datareader_dispatch(gap, src_prefix, dst_prefix,
                      &RtpsUdpDataLink::process_gap_i);
}

void
RtpsUdpDataLink::process_gap_i(const OpenDDS::RTPS::GapSubmessage& gap,
                               const RepoId& src, RtpsReaderMap::value_type& rr)
{
  WriterInfoMap::iterator wi = rr.second.remote_writers_.find(src);
  if (wi != rr.second.remote_writers_.end()) {
    SequenceRange sr;
    sr.first.setValue(gap.gapStart.high, gap.gapStart.low);
    SequenceNumber base;
    base.setValue(gap.gapList.bitmapBase.high, gap.gapList.bitmapBase.low);
    sr.second = base.previous();
    wi->second.recvd_.insert(sr);
    wi->second.recvd_.insert(base, gap.gapList.numBits,
                             gap.gapList.bitmap.get_buffer());
    //FUTURE: to support wait_for_acks(), notify DCPS layer of the GAP
  }
}

void
RtpsUdpDataLink::received(const OpenDDS::RTPS::HeartBeatSubmessage& heartbeat,
                          const GuidPrefix_t& src_prefix,
                          const GuidPrefix_t& dst_prefix)
{
  datareader_dispatch(heartbeat, src_prefix, dst_prefix,
                      &RtpsUdpDataLink::process_heartbeat_i);
}

void
RtpsUdpDataLink::process_heartbeat_i(
  const OpenDDS::RTPS::HeartBeatSubmessage& heartbeat,
  const RepoId& src, RtpsReaderMap::value_type& rr)
{
  WriterInfoMap::iterator wi = rr.second.remote_writers_.find(src);
  if (wi == rr.second.remote_writers_.end()) {
    // we may not be associated yet, even if the writer thinks we are
    return;
  }

  if (heartbeat.count.value <= wi->second.heartbeat_recvd_count_) {
    return;
  }

  wi->second.heartbeat_recvd_count_ = heartbeat.count.value;

  DisjointSequence& recvd = wi->second.recvd_;
  // don't attempt to nack anything below heartbeat.firstSN or above .lastSN
  if (!recvd.empty()) {
    SequenceNumber first;
    first.setValue(heartbeat.firstSN.high, heartbeat.firstSN.low);
    if (recvd.cumulative_ack() < first) {
      recvd.insert(SequenceRange(recvd.cumulative_ack(), first.previous()));
    }

    SequenceNumber last;
    last.setValue(heartbeat.lastSN.high, heartbeat.lastSN.low);
    if (recvd.last_ack() > last) {
      recvd.insert(SequenceRange(++SequenceNumber(last), recvd.last_ack()));
    }
    //FUTURE: to support wait_for_acks(), notify DCPS layer of the sequence
    //        numbers we no longer expect to receive due to HEARTBEAT
  }

  const bool final = heartbeat.smHeader.flags & 2 /* FLAG_F */,
    liveliness = heartbeat.smHeader.flags & 4 /* FLAG_L */;

  if (!final || (!liveliness && recvd.disjoint())) {
    wi->second.ack_pending_ = true;
    heartbeat_reply_.schedule(); // timer will invoke send_heartbeat_replies()
  }

  //FUTURE: support assertion of liveliness for MANUAL_BY_TOPIC
}

void
RtpsUdpDataLink::send_heartbeat_replies() // from DR to DW
{
  using namespace OpenDDS::RTPS;
  for (RtpsReaderMap::iterator rr = readers_.begin(); rr != readers_.end();
       ++rr) {
    WriterInfoMap& writers = rr->second.remote_writers_;
    for (WriterInfoMap::iterator wi = writers.begin(); wi != writers.end();
         ++wi) {

      // if we have some negative acknowledgements, we'll ask for a reply
      DisjointSequence& recvd = wi->second.recvd_;
      const bool nack = recvd.disjoint();

      if (wi->second.ack_pending_ || nack) {
        wi->second.ack_pending_ = false;

        SequenceNumber ack;
        CORBA::ULong num_bits = 1;
        LongSeq8 bitmap;
        bitmap.length(1);
        bitmap[0] = 0;

        if (!recvd.empty()) {
          ack = ++SequenceNumber(recvd.cumulative_ack());
          bitmap.length(std::min(CORBA::ULong(8),
                                 CORBA::ULong((recvd.last_ack().getValue() -
                                               ack.getValue() + 31) / 32)));
          recvd.to_bitmap(bitmap.get_buffer(), bitmap.length(), num_bits, true);
        }

        AckNackSubmessage acknack = {
          {ACKNACK, 1 /*FLAG_E*/ | (nack ? 0 : 2 /*FLAG_F*/), 0 /*length*/},
          rr->first.entityId,
          wi->first.entityId,
          { // SequenceNumberSet: acking bitmapBase - 1
            {ack.getHigh(), ack.getLow()},
            num_bits, bitmap
          },
          {++wi->second.acknack_count_}
        };

        size_t size = 0, padding = 0;
        gen_find_size(acknack, size, padding);
        acknack.smHeader.submessageLength =
          static_cast<CORBA::UShort>(size + padding) - SMHDR_SZ;
        // ok to visit these next two out-of-order, all of these are aligned
        gen_find_size(info_dst_, size, padding);
        gen_find_size(info_reply_, size, padding);

        ACE_Message_Block mb_acknack(size + padding); //TODO: allocators?
        // byte swapping is handled in the operator<<() implementation
        Serializer ser(&mb_acknack, false, Serializer::ALIGN_CDR);
        ser << info_dst_;
        ser << info_reply_;
        ser << acknack;

        if (!locators_.count(wi->first)) {
          //TODO: log error, don't know where to send it
        } else {
          std::set<ACE_INET_Addr> recipients;
          recipients.insert(locators_[wi->first]);
          send_strategy_->send_rtps_control(mb_acknack, recipients);
        }
      }
    }
  }
}

void
RtpsUdpDataLink::received(const OpenDDS::RTPS::AckNackSubmessage& acknack,
                          const GuidPrefix_t& src_prefix,
                          const GuidPrefix_t& dst_prefix)
{
  // local side is DW
  RepoId local;
  std::memcpy(local.guidPrefix, dst_prefix, sizeof(GuidPrefix_t));
  local.entityId = acknack.writerId; // can't be ENTITYID_UNKNOWN

  const RtpsWriterMap::iterator rw = writers_.find(local);
  if (rw == writers_.end()) {
    return;
  }

  RepoId remote;
  std::memcpy(remote.guidPrefix, src_prefix, sizeof(GuidPrefix_t));
  remote.entityId = acknack.readerId;

  const ReaderInfoMap::iterator ri = rw->second.remote_readers_.find(remote);
  if (ri == rw->second.remote_readers_.end()) {
    return;
  }

  if (acknack.count.value <= ri->second.acknack_recvd_count_) {
    return;
  }

  ri->second.acknack_recvd_count_ = acknack.count.value;

  const bool final = acknack.smHeader.flags & 2 /* FLAG_F */;

  // If this ACKNACK was final, the DR doesn't expect a reply, and therefore
  // we don't need to do anything further.
  if (!final) {
    ri->second.requested_changes_.push_back(acknack.readerSNState);
    nack_reply_.schedule(); // timer will invoke send_nack_replies()
  }
}

void
RtpsUdpDataLink::send_nack_replies()
{
  // Reply from local DW to remote DR: GAP or DATA
  using namespace OpenDDS::RTPS;
  typedef RtpsWriterMap::iterator rw_iter;
  for (rw_iter rw = writers_.begin(); rw != writers_.end(); rw++) {

    if (rw->second.send_buff_.is_nil() || rw->second.send_buff_->empty()) {
      continue; // no data available
    }

    // consolidate requests from N readers
    std::set<ACE_INET_Addr> recipients;
    DisjointSequence requests;
    RtpsWriter& writer = rw->second;

    typedef ReaderInfoMap::iterator ri_iter;
    const ri_iter end = writer.remote_readers_.end();
    for (ri_iter ri = writer.remote_readers_.begin(); ri != end; ++ri) {

      for (size_t i = 0; i < ri->second.requested_changes_.size(); ++i) {
        const SequenceNumberSet& sn_state = ri->second.requested_changes_[i];
        SequenceNumber base;
        base.setValue(sn_state.bitmapBase.high, sn_state.bitmapBase.low);
        requests.insert(base, sn_state.numBits, sn_state.bitmap.get_buffer());
      }

      if (ri->second.requested_changes_.size()) {
        if (locators_.count(ri->first)) {
          recipients.insert(locators_[ri->first]);
        }
        ri->second.requested_changes_.clear();
      }
    }

    if (requests.empty()) {
      continue;
    }

    std::vector<SequenceRange> ranges = requests.present_sequence_ranges();
    DisjointSequence gaps;
    SingleSendBuffer& sb = *rw->second.send_buff_;
    {
      ACE_GUARD(TransportSendBuffer::LockType, guard, sb.strategy_lock());
      RtpsUdpSendStrategy::OverrideToken ot =
        send_strategy_->override_destinations(recipients);
      for (size_t i = 0; i < ranges.size(); ++i) {
        sb.resend_i(ranges[i], &gaps);
      }
    }

    if (gaps.empty()) {
      continue;
    }

    // RTPS v2.1 8.3.7.4: the Gap sequence numbers are those in the range
    // [gapStart, gapListBase) and those in the SNSet.
    const SequenceNumber firstMissing = gaps.low(),
                         base = ++SequenceNumber(gaps.cumulative_ack());
    const SequenceNumber_t gapStart = {firstMissing.getHigh(),
                                       firstMissing.getLow()},
                           gapListBase = {base.getHigh(), base.getLow()};
    CORBA::ULong num_bits = 0;
    LongSeq8 bitmap;

    if (gaps.disjoint()) {
      bitmap.length(std::min(CORBA::ULong(8),
                             CORBA::ULong(gaps.high().getValue() -
                                          base.getValue()) / 32));
      gaps.to_bitmap(bitmap.get_buffer(), bitmap.length(), num_bits);

    } else {
      bitmap.length(1);
      bitmap[0] = 0;
      num_bits = 1;
    }

    GapSubmessage gap = {
      {GAP, 1 /*FLAG_E*/, 0 /*length determined below*/},
      ENTITYID_UNKNOWN, // readerId: applies to all matched readers
      rw->first.entityId,
      gapStart,
      {gapListBase, num_bits, bitmap}
    };

    size_t size = 0, padding = 0;
    gen_find_size(gap, size, padding);
    gap.smHeader.submessageLength =
      static_cast<CORBA::UShort>(size + padding) - SMHDR_SZ;

    ACE_Message_Block mb_gap(size + padding); //TODO: allocators?
    // byte swapping is handled in the operator<<() implementation
    Serializer ser(&mb_gap, false, Serializer::ALIGN_CDR);
    ser << gap;
    send_strategy_->send_rtps_control(mb_gap, recipients);
  }
}

void
RtpsUdpDataLink::send_heartbeats()
{
  using namespace OpenDDS::RTPS;
  std::vector<HeartBeatSubmessage> subm;
  std::set<ACE_INET_Addr> recipients;

  typedef RtpsWriterMap::iterator rw_iter;
  for (rw_iter rw = writers_.begin(); rw != writers_.end(); rw++) {

    if (rw->second.send_buff_.is_nil() || rw->second.send_buff_->empty()) {
      continue; // no data available -> no need to heartbeat (8.4.2.2.3)
    }

    const SequenceNumber firstSN = rw->second.send_buff_->low(),
      lastSN = rw->second.send_buff_->high();

    const HeartBeatSubmessage hb = {
      {HEARTBEAT, 1 /*FLAG_E*/ | 2 /*FLAG_F*/, HEARTBEAT_SZ},
      ENTITYID_UNKNOWN, // any matched reader may be interested in this
      rw->first.entityId,
      {firstSN.getHigh(), firstSN.getLow()},
      {lastSN.getHigh(), lastSN.getLow()},
      {++rw->second.heartbeat_count_}
    };
    subm.push_back(hb);

    typedef ReaderInfoMap::iterator ri_iter;
    ri_iter end = rw->second.remote_readers_.end();
    for (ri_iter ri = rw->second.remote_readers_.begin(); ri != end; ++ri) {
      if (locators_.count(ri->first)) {
        recipients.insert(locators_[ri->first]);
      }
    }
  }

  if (!subm.empty()) {
    ACE_Message_Block mb((HEARTBEAT_SZ + SMHDR_SZ) * subm.size()); //TODO: allocators?
    // byte swapping is handled in the operator<<() implementation
    Serializer ser(&mb, false, Serializer::ALIGN_CDR);
    for (size_t i = 0; i < subm.size(); ++i) {
      if (!(ser << subm[i])) {
        ACE_DEBUG((LM_ERROR, "(%P|%t) RtpsUdpDataLink::send_heartbeats() - "
          "failed to serialize HEARTBEAT submessage %B\n", i));
        return;
      }
    }
    send_strategy_->send_rtps_control(mb, recipients);
  }
}

void
RtpsUdpDataLink::TimedDelay::schedule()
{
  if (!scheduled_) {
    const long timer = outer_->get_reactor()->schedule_timer(this, 0, timeout_);

    if (timer == -1) {
      ACE_DEBUG((LM_ERROR, "(%P|%t) RtpsUdpDataLink::TimedDelay::schedule "
        "failed to schedule timer %p\n", ACE_TEXT("")));
    } else {
      scheduled_ = true;
    }
  }
}

void
RtpsUdpDataLink::TimedDelay::cancel()
{
  if (scheduled_) {
    outer_->get_reactor()->cancel_timer(this);
    scheduled_ = false;
  }
}

void
RtpsUdpDataLink::HeartBeat::enable()
{
  if (!enabled_) {
    const ACE_Time_Value& per = outer_->config_->heartbeat_period_;
    const long timer = outer_->get_reactor()->schedule_timer(this, 0, per, per);

    if (timer == -1) {
      ACE_DEBUG((LM_ERROR, "(%P|%t) RtpsUdpDataLink::HeartBeat::enable"
        " failed to schedule timer %p\n", ACE_TEXT("")));
    } else {
      enabled_ = true;
    }
  }
}

void
RtpsUdpDataLink::HeartBeat::disable()
{
  if (enabled_) {
    outer_->get_reactor()->cancel_timer(this);
    enabled_ = false;
  }
}

} // namespace DCPS
} // namespace OpenDDS
