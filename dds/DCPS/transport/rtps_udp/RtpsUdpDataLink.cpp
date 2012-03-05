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

#include "dds/DCPS/transport/framework/TransportCustomizedElement.h"
#include "dds/DCPS/transport/framework/TransportSendElement.h"
#include "dds/DCPS/transport/framework/TransportSendControlElement.h"

#include "dds/DCPS/RTPS/BaseMessageUtils.h"
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
    handshake_condition_(lock_),
    nack_reply_(this, &RtpsUdpDataLink::send_nack_replies,
                config->nak_response_delay_),
    heartbeat_reply_(this, &RtpsUdpDataLink::send_heartbeat_replies,
                     config->heartbeat_response_delay_),
    heartbeat_(this)
{
  std::memcpy(local_prefix_, local_prefix, sizeof(GuidPrefix_t));
}

bool
RtpsUdpDataLink::open(const ACE_SOCK_Dgram& unicast_socket)
{
  unicast_socket_ = unicast_socket;

#ifdef ACE_HAS_IPV6
  ACE_INET_Addr uni_addr;
  if (0 != unicast_socket_.get_local_addr(uni_addr)) {
    VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::open: "
          "ACE_SOCK_Dgram::get_local_addr %p\n", ACE_TEXT("")));
  } else {
    unicast_socket_type_ = uni_addr.get_type();
    const unsigned short any_port = 0;
    if (unicast_socket_type_ == AF_INET6) {
      const ACE_UINT32 any_addr = INADDR_ANY;
      ACE_INET_Addr alt_addr(any_port, any_addr);
      if (0 != ipv6_alternate_socket_.open(alt_addr)) {
        VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::open: "
              "ACE_SOCK_Dgram::open %p\n", ACE_TEXT("alternate IPv4")));
      }
    } else {
      ACE_INET_Addr alt_addr(any_port, ACE_IPV6_ANY, AF_INET6);
      if (0 != ipv6_alternate_socket_.open(alt_addr)) {
        VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::open: "
              "ACE_SOCK_Dgram::open %p\n", ACE_TEXT("alternate IPv6")));
      }
    }
  }
#endif

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

  // Set up info_dst_ and info_reply_ messages for use with ACKNACKS
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
  RTPS::address_to_bytes(info_reply_.unicastLocatorList[0].address,
                         config_->local_address_);
  if (config_->use_multicast_) {
    info_reply_.smHeader.flags |= 2 /*FLAG_M*/;
    info_reply_.multicastLocatorList.length(1);
    info_reply_.multicastLocatorList[0].kind =
      (config_->multicast_group_address_.get_type() == AF_INET6)
      ? LOCATOR_KIND_UDPv6 : LOCATOR_KIND_UDPv4;
    info_reply_.multicastLocatorList[0].port =
      config_->multicast_group_address_.get_port_number();
    RTPS::address_to_bytes(info_reply_.multicastLocatorList[0].address,
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

#ifdef ACE_HAS_IPV6
ACE_SOCK_Dgram&
RtpsUdpDataLink::socket_for(int address_type)
{
  return (address_type == unicast_socket_type_)
    ? unicast_socket_
    : ipv6_alternate_socket_;
}
#endif

void
RtpsUdpDataLink::add_locator(const RepoId& remote_id,
                             const ACE_INET_Addr& address,
                             bool requires_inline_qos)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  locators_[remote_id] = RemoteInfo(address, requires_inline_qos);
}

void
RtpsUdpDataLink::get_locators(const RepoId& local_id,
                              std::set<ACE_INET_Addr>& addrs) const
{
  using std::map;
  typedef map<RepoId, RemoteInfo, GUID_tKeyLessThan>::const_iterator iter_t;

  if (local_id == GUID_UNKNOWN) {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    for (iter_t iter = locators_.begin(); iter != locators_.end(); ++iter) {
      addrs.insert(iter->second.addr_);
    }
    return;
  }

  const GUIDSeq_var peers = peer_ids(local_id);
  if (!peers.ptr()) {
    return;
  }
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  for (CORBA::ULong i = 0; i < peers->length(); ++i) {
    const iter_t iter = locators_.find(peers[i]);
    if (iter == locators_.end()) {
      const GuidConverter conv(peers[i]);
      ACE_DEBUG((LM_ERROR, "(%P|%t) RtpsUdpDataLink::get_locators() - "
        "no locator found for peer %C\n", std::string(conv).c_str()));
    } else {
      addrs.insert(iter->second.addr_);
    }
  }
}

void
RtpsUdpDataLink::associated(const RepoId& local_id, const RepoId& remote_id,
                            bool local_reliable, bool remote_reliable,
                            bool local_durable)
{
  if (!local_reliable) {
    return;
  }

  bool enable_heartbeat = false;

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  const GuidConverter conv(local_id);
  const EntityKind kind = conv.entityKind();
  if (kind == KIND_WRITER && remote_reliable) {
    writers_[local_id].remote_readers_[remote_id];
    enable_heartbeat = true;

  } else if (kind == KIND_READER) {
    RtpsReaderMap::iterator rr = readers_.find(local_id);
    if (rr == readers_.end()) {
      rr = readers_.insert(RtpsReaderMap::value_type(local_id, RtpsReader()))
        .first;
      rr->second.durable_ = local_durable;
    }
    rr->second.remote_writers_[remote_id];
    reader_index_.insert(RtpsReaderIndex::value_type(remote_id, rr));
  }

  g.release();
  if (enable_heartbeat) {
    heartbeat_.enable();
  }
}

bool
RtpsUdpDataLink::wait_for_handshake(const RepoId& local_id,
                                    const RepoId& remote_id)
{
  ACE_Time_Value abs_timeout = ACE_OS::gettimeofday()
    + config_->handshake_timeout_;
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  while (!handshake_done(local_id, remote_id)) {
    if (handshake_condition_.wait(&abs_timeout) == -1) {
      return false;
    }
  }
  return true;
}

bool
RtpsUdpDataLink::handshake_done(const RepoId& local_id, const RepoId& remote_id)
{
  const GuidConverter conv(local_id);
  const EntityKind kind = conv.entityKind();
  if (kind == KIND_WRITER) {
    RtpsWriterMap::iterator rw = writers_.find(local_id);
    if (rw == writers_.end()) {
      return true; // not reliable, no handshaking
    }
    ReaderInfoMap::iterator ri = rw->second.remote_readers_.find(remote_id);
    if (ri == rw->second.remote_readers_.end()) {
      return true; // not reliable, no handshaking
    }
    return ri->second.handshake_done_;

  } else if (kind == KIND_READER) {
    return true; // no handshaking for local reader
  }
  return false;
}

void
RtpsUdpDataLink::release_reservations_i(const RepoId& remote_id,
                                        const RepoId& local_id)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  using std::pair;
  const GuidConverter conv(local_id);
  const EntityKind kind = conv.entityKind();
  if (kind == KIND_WRITER) {
    const RtpsWriterMap::iterator rw = writers_.find(local_id);

    if (rw != writers_.end()) {
      rw->second.remote_readers_.erase(remote_id);

      if (rw->second.remote_readers_.empty()) {
        writers_.erase(rw);
      }
    }

  } else if (kind == KIND_READER) {
    const RtpsReaderMap::iterator rr = readers_.find(local_id);

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
  unicast_socket_.close();
  multicast_socket_.close();
}


// Implementing MultiSendBuffer nested class

void
RtpsUdpDataLink::MultiSendBuffer::retain_all(RepoId pub_id)
{
  ACE_GUARD(ACE_Thread_Mutex, g, outer_->lock_);
  const RtpsWriterMap::iterator wi = outer_->writers_.find(pub_id);
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

  ACE_GUARD(ACE_Thread_Mutex, g, outer_->lock_);
  const RtpsWriterMap::iterator wi = outer_->writers_.find(pub_id);
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


// Support for the send() data handling path

TransportQueueElement*
RtpsUdpDataLink::customize_queue_element(TransportQueueElement* element)
{
  const ACE_Message_Block* msg = element->msg();
  if (!msg) {
    return element;
  }

  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, 0);

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
    if (RtpsSampleHeader::control_message_supported(tsce->header().message_id_)) {
      data = msg->cont()->duplicate();
      // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader
      RtpsSampleHeader::populate_data_control_submessages(
                subm, *tsce, this->requires_inline_qos(tsce->publication_id()));
    } else {
      element->data_dropped(true /*dropped_by_transport*/);
      return 0;
    }

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
RtpsUdpDataLink::requires_inline_qos(const PublicationId& pub_id)
{
  if (this->force_inline_qos_) {
    // Force true for testing purposes
    return true;
  } else {
    const GUIDSeq_var peers = peer_ids(pub_id);
    if (!peers.ptr()) {
      return false;
    }
    typedef std::map<RepoId, RemoteInfo, GUID_tKeyLessThan>::iterator iter_t;
    for (CORBA::ULong i = 0; i < peers->length(); ++i) {
      const iter_t iter = locators_.find(peers[i]);
      if (iter != locators_.end() && iter->second.requires_inline_qos_) {
        return true;
      }
    }
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


// DataReader's side of Reliability

void
RtpsUdpDataLink::received(const OpenDDS::RTPS::DataSubmessage& data,
                          const GuidPrefix_t& src_prefix)
{
  datareader_dispatch(data, src_prefix, &RtpsUdpDataLink::process_data_i);
}

void
RtpsUdpDataLink::process_data_i(const OpenDDS::RTPS::DataSubmessage& data,
                                const RepoId& src,
                                RtpsReaderMap::value_type& rr)
{
  const WriterInfoMap::iterator wi = rr.second.remote_writers_.find(src);
  if (wi != rr.second.remote_writers_.end()) {
    SequenceNumber seq;
    seq.setValue(data.writerSN.high, data.writerSN.low);
    wi->second.recvd_.insert(seq);
    wi->second.frags_.erase(seq);
  }
}

void
RtpsUdpDataLink::received(const OpenDDS::RTPS::GapSubmessage& gap,
                          const GuidPrefix_t& src_prefix)
{
  datareader_dispatch(gap, src_prefix, &RtpsUdpDataLink::process_gap_i);
}

void
RtpsUdpDataLink::process_gap_i(const OpenDDS::RTPS::GapSubmessage& gap,
                               const RepoId& src, RtpsReaderMap::value_type& rr)
{
  const WriterInfoMap::iterator wi = rr.second.remote_writers_.find(src);
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
                          const GuidPrefix_t& src_prefix)
{
  datareader_dispatch(heartbeat, src_prefix,
                      &RtpsUdpDataLink::process_heartbeat_i);
}

void
RtpsUdpDataLink::process_heartbeat_i(
  const OpenDDS::RTPS::HeartBeatSubmessage& heartbeat,
  const RepoId& src, RtpsReaderMap::value_type& rr)
{
  const WriterInfoMap::iterator wi = rr.second.remote_writers_.find(src);
  if (wi == rr.second.remote_writers_.end()) {
    // we may not be associated yet, even if the writer thinks we are
    return;
  }

  WriterInfo& info = wi->second;

  if (heartbeat.count.value <= info.heartbeat_recvd_count_) {
    return;
  }
  info.heartbeat_recvd_count_ = heartbeat.count.value;

  SequenceNumber& first = info.hb_range_.first;
  first.setValue(heartbeat.firstSN.high, heartbeat.firstSN.low);
  SequenceNumber& last = info.hb_range_.second;
  last.setValue(heartbeat.lastSN.high, heartbeat.lastSN.low);

  DisjointSequence& recvd = info.recvd_;
  // the cumulative ack may only increase ("once acked, always acked" rule) and
  // the cumultaive ack must be at least first - 1
  if (!recvd.empty() && recvd.cumulative_ack() < first.previous()) {
    recvd.insert(SequenceRange(recvd.cumulative_ack(), first.previous()));
  }
  //FUTURE: to support wait_for_acks(), notify DCPS layer of the sequence
  //        numbers (dropped) we no longer expect to receive due to HEARTBEAT

  if (!rr.second.durable_ && info.initial_hb_) {
    recvd.insert(SequenceRange(SequenceNumber(),
                               recvd.empty() ? last.previous() : recvd.low()));
  }
  info.initial_hb_ = false;

  const bool final = heartbeat.smHeader.flags & 2 /* FLAG_F */,
    liveliness = heartbeat.smHeader.flags & 4 /* FLAG_L */;

  if (!final || (!liveliness && (info.should_nack() ||
      recv_strategy_->has_fragments(info.hb_range_, wi->first)))) {
    info.ack_pending_ = true;
    heartbeat_reply_.schedule(); // timer will invoke send_heartbeat_replies()
  }

  //FUTURE: support assertion of liveliness for MANUAL_BY_TOPIC
}

bool
RtpsUdpDataLink::WriterInfo::should_nack() const
{
  return recvd_.disjoint()
    || (!recvd_.empty() && (recvd_.high() < hb_range_.second ||
                            recvd_.low() > hb_range_.first));
}

void
RtpsUdpDataLink::send_heartbeat_replies() // from DR to DW
{
  using namespace OpenDDS::RTPS;
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  for (RtpsReaderMap::iterator rr = readers_.begin(); rr != readers_.end();
       ++rr) {
    WriterInfoMap& writers = rr->second.remote_writers_;
    for (WriterInfoMap::iterator wi = writers.begin(); wi != writers.end();
         ++wi) {

      // if we have some negative acknowledgements, we'll ask for a reply
      DisjointSequence& recvd = wi->second.recvd_;
      const bool nack = recvd.disjoint();
      //TODO: Need to account for new nacking logic in WriterInfo::should_nack()

      if (wi->second.ack_pending_ || nack) {
        wi->second.ack_pending_ = false;

        SequenceNumber ack;
        CORBA::ULong num_bits = 1;
        LongSeq8 bitmap;
        bitmap.length(1);
        bitmap[0] = 0;

        if (!recvd.empty()) {
          ack = ++SequenceNumber(recvd.cumulative_ack());
          if (recvd.low() > 1) {
            // since the "ack" really is cumulative, we need to make
            // sure that a lower discontinuity is not possible later
            recvd.insert(SequenceRange(1, recvd.cumulative_ack()));
          }
        }

        if (recvd.disjoint()) {
          bitmap.length(std::min(CORBA::ULong(8),
                                 CORBA::ULong((recvd.last_ack().getValue() -
                                               ack.getValue() + 31) / 32)));
          recvd.to_bitmap(bitmap.get_buffer(), bitmap.length(), num_bits, true);

          // If the receive strategy is holding any fragments, those should
          // not be "nacked" in the ACKNACK reply.  They will be accounted for
          // in the NACK_FRAG(s) instead.
          recv_strategy_->remove_frags_from_bitmap(bitmap.get_buffer(),
                                                   num_bits, ack, wi->first);
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

        std::vector<NackFragSubmessage> nack_frags;
        size += generate_nack_frags(nack_frags, wi->second, wi->first);

        ACE_Message_Block mb_acknack(size + padding); //FUTURE: allocators?
        // byte swapping is handled in the operator<<() implementation
        Serializer ser(&mb_acknack, false, Serializer::ALIGN_CDR);
        std::memcpy(info_dst_.guidPrefix, wi->first.guidPrefix,
                    sizeof(GuidPrefix_t));
        ser << info_dst_;
        ser << info_reply_;
        ser << acknack;
        for (size_t i = 0; i < nack_frags.size(); ++i) {
          nack_frags[i].readerId = rr->first.entityId;
          nack_frags[i].writerId = wi->first.entityId;
          ser << nack_frags[i]; // always 4-byte aligned
        }

        if (!locators_.count(wi->first)) {
          if (Transport_debug_level) {
            const GuidConverter conv(wi->first);
            ACE_DEBUG((LM_ERROR,
              "(%P|%t) RtpsUdpDataLink::send_heartbeat_replies() - "
              "no locator for remote %C\n", std::string(conv).c_str()));
          }
        } else {
          std::set<ACE_INET_Addr> recipients;
          recipients.insert(locators_[wi->first].addr_);
          send_strategy_->send_rtps_control(mb_acknack, recipients);
        }
      }
    }
  }
}

size_t
RtpsUdpDataLink::generate_nack_frags(std::vector<RTPS::NackFragSubmessage>& nf,
                                     WriterInfo& wi, const RepoId& pub_id)
{
  typedef std::map<SequenceNumber, RTPS::FragmentNumber_t>::iterator iter_t;
  typedef RtpsUdpReceiveStrategy::FragmentInfo::value_type Frag_t;
  RtpsUdpReceiveStrategy::FragmentInfo frag_info;

  // Populate frag_info with two possible sources of NackFrags:
  // 1. sequence #s in the reception gaps that we have partially received
  std::vector<SequenceRange> missing = wi.recvd_.missing_sequence_ranges();
  for (size_t i = 0; i < missing.size(); ++i) {
    recv_strategy_->has_fragments(missing[i], pub_id, &frag_info);
  }
  for (size_t i = 0; i < frag_info.size(); ++i) {
    // If we've received a HeartbeatFrag, we know the last (available) frag #
    const iter_t iter = wi.frags_.find(frag_info[i].first);
    if (iter != wi.frags_.end()) {
      extend_bitmap_range(frag_info[i].second, iter->second.value);
    }
  }

  // 2. sequence #s outside the recvd_ gaps for which we have a HeartbeatFrag
  const iter_t low = wi.frags_.lower_bound(wi.recvd_.cumulative_ack()),
              high = wi.frags_.upper_bound(wi.recvd_.last_ack()),
               end = wi.frags_.end();
  for (iter_t iter = wi.frags_.begin(); iter != end; ++iter) {
    if (iter == low) {
      // skip over the range covered by step #1 above
      if (high == end) {
        break;
      }
      iter = high;
    }

    const SequenceRange range(iter->first, iter->first);
    if (recv_strategy_->has_fragments(range, pub_id, &frag_info)) {
      extend_bitmap_range(frag_info.back().second, iter->second.value);
    } else {
      // it was not in the recv strategy, so the entire range is "missing"
      frag_info.push_back(Frag_t(iter->first, RTPS::FragmentNumberSet()));
      RTPS::FragmentNumberSet& fnSet = frag_info.back().second;
      fnSet.bitmapBase.value = 1;
      fnSet.numBits = std::min(CORBA::ULong(256), iter->second.value);
      fnSet.bitmap.length((fnSet.numBits + 31) / 32);
      for (CORBA::ULong i = 0; i < fnSet.bitmap.length(); ++i) {
        fnSet.bitmap[i] = 0xFFFFFFFF;
      }
    }
  }

  if (frag_info.empty()) {
    return 0;
  }

  const RTPS::NackFragSubmessage nackfrag_prototype = {
    {RTPS::NACK_FRAG, 1 /*FLAG_E*/, 0 /* length set below */},
    ENTITYID_UNKNOWN, // readerId will be filled-in by send_heartbeat_replies()
    ENTITYID_UNKNOWN, // writerId will be filled-in by send_heartbeat_replies()
    {0, 0}, // writerSN set below
    RTPS::FragmentNumberSet(), // fragmentNumberState set below
    {0} // count set below
  };

  size_t size = 0, padding = 0;
  for (size_t i = 0; i < frag_info.size(); ++i) {
    nf.push_back(nackfrag_prototype);
    RTPS::NackFragSubmessage& nackfrag = nf.back();
    nackfrag.writerSN.low = frag_info[i].first.getLow();
    nackfrag.writerSN.high = frag_info[i].first.getHigh();
    nackfrag.fragmentNumberState = frag_info[i].second;
    nackfrag.count.value = ++wi.nackfrag_count_;
    const size_t before_size = size;
    gen_find_size(nackfrag, size, padding);
    nackfrag.smHeader.submessageLength =
      static_cast<CORBA::UShort>(size - before_size) - RTPS::SMHDR_SZ;
  }
  return size;
}

void
RtpsUdpDataLink::extend_bitmap_range(OpenDDS::RTPS::FragmentNumberSet& fnSet,
                                     CORBA::ULong extent)
{
  if (extent < fnSet.bitmapBase.value) {
    return; // can't extend to some number under the base
  }
  const CORBA::ULong index = std::min(CORBA::ULong(255),
                                      extent - fnSet.bitmapBase.value),
                     len = (index + 31) / 32;
  if (index < fnSet.numBits) {
    return; // bitmap already extends past "extent"
  }
  fnSet.bitmap.length(len);
  DisjointSequence::fill_bitmap_range(fnSet.numBits, index,
                                      fnSet.bitmap.get_buffer(), len,
                                      fnSet.numBits);
}

void
RtpsUdpDataLink::received(const OpenDDS::RTPS::HeartBeatFragSubmessage& hb_frag,
                          const GuidPrefix_t& src_prefix)
{
  datareader_dispatch(hb_frag, src_prefix, &RtpsUdpDataLink::process_hb_frag_i);
}

void
RtpsUdpDataLink::process_hb_frag_i(
  const OpenDDS::RTPS::HeartBeatFragSubmessage& hb_frag,
  const RepoId& src, RtpsReaderMap::value_type& rr)
{
  WriterInfoMap::iterator wi = rr.second.remote_writers_.find(src);
  if (wi == rr.second.remote_writers_.end()) {
    // we may not be associated yet, even if the writer thinks we are
    return;
  }

  if (hb_frag.count.value <= wi->second.hb_frag_recvd_count_) {
    return;
  }

  wi->second.hb_frag_recvd_count_ = hb_frag.count.value;

  SequenceNumber seq;
  seq.setValue(hb_frag.writerSN.high, hb_frag.writerSN.low);

  // If seq is outside the heartbeat range or we haven't completely received
  // it yet, send a NackFrag along with the AckNack.  The heartbeat range needs
  // to be checked first because recvd_ contains the numbers below the
  // heartbeat range (so that we don't NACK those).
  if (seq < wi->second.hb_range_.first || seq > wi->second.hb_range_.second
      || !wi->second.recvd_.contains(seq)) {
    wi->second.frags_[seq] = hb_frag.lastFragmentNum;
    wi->second.ack_pending_ = true;
    heartbeat_reply_.schedule(); // timer will invoke send_heartbeat_replies()
  }
}


// DataWriter's side of Reliability

void
RtpsUdpDataLink::received(const OpenDDS::RTPS::AckNackSubmessage& acknack,
                          const GuidPrefix_t& src_prefix)
{
  // local side is DW
  RepoId local;
  std::memcpy(local.guidPrefix, local_prefix_, sizeof(GuidPrefix_t));
  local.entityId = acknack.writerId; // can't be ENTITYID_UNKNOWN

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  const RtpsWriterMap::iterator rw = writers_.find(local);
  if (rw == writers_.end()) {
    if (Transport_debug_level > 5) {
      GuidConverter local_conv(local);
      ACE_DEBUG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::received(ACKNACK) "
        "WARNING local %C no RtpsWriter\n", std::string(local_conv).c_str()));
    }
    return;
  }

  RepoId remote;
  std::memcpy(remote.guidPrefix, src_prefix, sizeof(GuidPrefix_t));
  remote.entityId = acknack.readerId;

  if (Transport_debug_level > 5) {
    GuidConverter local_conv(local), remote_conv(remote);
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::received(ACKNACK) "
      "local %C remote %C\n", std::string(local_conv).c_str(),
      std::string(remote_conv).c_str()));
  }

  const ReaderInfoMap::iterator ri = rw->second.remote_readers_.find(remote);
  if (ri == rw->second.remote_readers_.end()) {
    VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::received(ACKNACK) "
      "WARNING ReaderInfo not found\n"));
    return;
  }

  if (acknack.count.value <= ri->second.acknack_recvd_count_) {
    VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::received(ACKNACK) "
      "WARNING Count indicates duplicate, dropping\n"));
    return;
  }

  ri->second.acknack_recvd_count_ = acknack.count.value;

  if (!ri->second.handshake_done_) {
    ri->second.handshake_done_ = true;
    handshake_condition_.broadcast();
  }

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
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  // Reply from local DW to remote DR: GAP or DATA
  using namespace OpenDDS::RTPS;
  typedef RtpsWriterMap::iterator rw_iter;
  for (rw_iter rw = writers_.begin(); rw != writers_.end(); ++rw) {

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
          recipients.insert(locators_[ri->first].addr_);
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
      const RtpsUdpSendStrategy::OverrideToken ot =
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

    ACE_Message_Block mb_gap(size + padding); //FUTURE: allocators?
    // byte swapping is handled in the operator<<() implementation
    Serializer ser(&mb_gap, false, Serializer::ALIGN_CDR);
    ser << gap;
    send_strategy_->send_rtps_control(mb_gap, recipients);
  }
}

void
RtpsUdpDataLink::send_heartbeats()
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  if (writers_.empty()) {
    heartbeat_.disable();
  }

  using namespace OpenDDS::RTPS;
  std::vector<HeartBeatSubmessage> subm;
  std::set<ACE_INET_Addr> recipients;

  typedef RtpsWriterMap::iterator rw_iter;
  for (rw_iter rw = writers_.begin(); rw != writers_.end(); ++rw) {

    const bool has_data = !rw->second.send_buff_.is_nil()
                          && !rw->second.send_buff_->empty();
    bool final = true;

    typedef ReaderInfoMap::iterator ri_iter;
    const ri_iter end = rw->second.remote_readers_.end();
    for (ri_iter ri = rw->second.remote_readers_.begin(); ri != end; ++ri) {
      if ((has_data || !ri->second.handshake_done_)
          && locators_.count(ri->first)) {
        recipients.insert(locators_[ri->first].addr_);
        if (final && !ri->second.handshake_done_) {
          final = false;
        }
      }
    }

    if (final && !has_data) {
      continue;
    }

    const SequenceNumber firstSN = has_data ? rw->second.send_buff_->low() : 1,
      lastSN = has_data ? rw->second.send_buff_->high() : 1;

    const HeartBeatSubmessage hb = {
      {HEARTBEAT, 1 /*FLAG_E*/ | (final ? 2 /*FLAG_F*/ : 0), HEARTBEAT_SZ},
      ENTITYID_UNKNOWN, // any matched reader may be interested in this
      rw->first.entityId,
      {firstSN.getHigh(), firstSN.getLow()},
      {lastSN.getHigh(), lastSN.getLow()},
      {++rw->second.heartbeat_count_}
    };
    subm.push_back(hb);
  }

  if (!subm.empty()) {
    ACE_Message_Block mb((HEARTBEAT_SZ + SMHDR_SZ) * subm.size()); //FUTURE: allocators?
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


// Implementing TimedDelay and HeartBeat nested classes (for ACE timers)

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
