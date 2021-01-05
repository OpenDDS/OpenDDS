// Test for the Reliability features (Heartbeat, AckNack, Gap) in the RtpsUdp
// OpenDDS transport implementation.

#include "dds/DCPS/transport/rtps_udp/RtpsUdpInst.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "../RtpsUtils.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"

#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/MessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include "dds/DCPS/RTPS/GuidGenerator.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/DisjointSequence.h"
#include "dds/DCPS/SendStateDataSampleList.h"
#include "dds/DCPS/DataSampleElement.h"
#include "dds/DCPS/Message_Block_Ptr.h"

#include <tao/Exception.h>

#include <ace/OS_main.h>
#include <ace/Thread_Manager.h>
#include <ace/Reactor.h>
#include <ace/SOCK_Dgram.h>

#include <cstdlib>
#include <typeinfo>
#include <exception>
#include <iostream>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

const bool host_is_bigendian = !ACE_CDR_BYTE_ORDER;

SequenceNumber makeSN(CORBA::Long high, CORBA::ULong low) {
  SequenceNumber result;
  result.setValue(high, low);
  return result;
}

struct SimpleTC: TransportClient {
  explicit SimpleTC(const RepoId& local) : local_id_(local), mutex_(), cond_(mutex_) {}

  void transport_assoc_done(int flags, const RepoId& remote) {
    if (!(flags & ASSOC_OK)) {
      return;
    }
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    associated_.insert(remote);
    cond_.broadcast();
  }

  void wait_for_assoc(const RepoId& remote) {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    while (associated_.find(remote) == associated_.end()) {
      cond_.wait(mutex_);
    }
  }

  using TransportClient::enable_transport;
  using TransportClient::associate;
  using TransportClient::disassociate;
  using TransportClient::send;
  using TransportClient::connection_info;

  const RepoId& get_repo_id() const { return local_id_; }
  DDS::DomainId_t domain_id() const { return 0; }
  bool check_transport_qos(const TransportInst&) { return true; }
  CORBA::Long get_priority_value(const AssociationData&) const { return 0; }

  RepoId local_id_;
  RepoIdSet associated_;
  ACE_Thread_Mutex mutex_;
  ACE_Condition<ACE_Thread_Mutex> cond_;
};

struct SimpleDataReader: SimpleTC, TransportReceiveListener {
  explicit SimpleDataReader(const RepoId& sub_id)
    : SimpleTC(sub_id), have_frag_(false) {

      // The reference count is explicited incremented to avoid been explcitly deleted
      // via the RcHandle<TransportClient> because the object is always been created
      // on the stack.
      RcObject::_add_ref();
    }

  void data_received(const ReceivedDataSample& sample)
  {
    ACE_DEBUG((LM_INFO, "data_received with seq#: %q\n",
      sample.header_.sequence_.getValue()));
    recvd_.insert(sample.header_.sequence_);
    if (sample.header_.sequence_ == 5) {
      recvd_.insert(4); // 4 is a deliberate GAP
    }
    if (sample.header_.sequence_ == 6) { // reassembled from DATA_FRAG
      if (sample.header_.message_length_ != 3 * 1024
          || sample.sample_->total_length() != 3 * 1024) {
        ACE_ERROR((LM_ERROR, "ERROR: unexpected reassembled sample length\n"));
      }
      if (have_frag_) {
        ACE_ERROR((LM_ERROR, "ERROR: duplicate delivery from DATA_FRAG\n"));
      }
      have_frag_ = true;
    }
  }

  void notify_subscription_disconnected(const WriterIdSeq&) {}
  void notify_subscription_reconnected(const WriterIdSeq&) {}
  void notify_subscription_lost(const WriterIdSeq&) {}
  void remove_associations(const WriterIdSeq&, bool) {}

  DisjointSequence recvd_;
  bool have_frag_;
};

class DDS_TEST
{
public:

  static void list_set(DataSampleElement &element, SendStateDataSampleList &list)
  {
    list.head_ = &element;
    list.tail_ = &element;
    list.size_ = 1;
  }
};


struct SimpleDataWriter: SimpleTC, TransportSendListener {
  explicit SimpleDataWriter(const RepoId& pub_id)
    : SimpleTC(pub_id)
    , dsle_(pub_id, this, OpenDDS::DCPS::PublicationInstance_rch())
  {
    DDS_TEST::list_set(dsle_, list_);
    dsle_.get_header().message_id_ = SAMPLE_DATA;
    dsle_.get_header().message_length_ = 8;
    dsle_.get_header().byte_order_ = ACE_CDR_BYTE_ORDER;
    payload_.init(dsle_.get_header().message_length_);
    const ACE_CDR::ULong encap = 0x00000100, // {CDR_LE, options} in LE format
      data = 0xDCBADCBA;
    Serializer ser(&payload_, host_is_bigendian, Serializer::ALIGN_CDR);
    ser << encap;
    ser << data;

    // The reference count is explicited incremented to avoid been explcitly deleted
    // via the RcHandle<TransportClient> because the object is always been created
    // on the stack.
    RcObject::_add_ref();
  }

  ~SimpleDataWriter()
  {
  }

  void send_data(const SequenceNumber& seq, const RepoId& dst = GUID_UNKNOWN)
  {
    dsle_.get_header().sequence_ = seq;
    dsle_.get_header().historic_sample_ = dst != GUID_UNKNOWN;
    if (dst != GUID_UNKNOWN) {
      dsle_.set_num_subs(1);
      dsle_.set_sub_id(0, dst);
    }
    Message_Block_Ptr sample(new ACE_Message_Block(DataSampleHeader::max_marshaled_size()));
    dsle_.set_sample(move(sample));
    *dsle_.get_sample() << dsle_.get_header();
    dsle_.get_sample()->cont(payload_.duplicate());
    ACE_DEBUG((LM_INFO, "sending with seq#: %q %d\n", seq.getValue(), dst != GUID_UNKNOWN));
    send(list_);
  }

  void
  set_header_fields(DataSampleHeader& dsh,
                    size_t size,
                    const RepoId& reader,
                    SequenceNumber& sequence,
                    bool historic_sample,
                    MessageId id)
  {
    dsh.message_id_ = id;
    dsh.byte_order_ = ACE_CDR_BYTE_ORDER;
    dsh.message_length_ = static_cast<ACE_UINT32>(size);
    dsh.publication_id_ = local_id_;

    if (id != END_HISTORIC_SAMPLES &&
        (reader == GUID_UNKNOWN ||
        sequence == SequenceNumber::SEQUENCENUMBER_UNKNOWN())) {
    }

    if (historic_sample && reader != GUID_UNKNOWN) {
      // retransmit with same seq# for durability
      dsh.historic_sample_ = true;
    }

    dsh.sequence_ = sequence;

    const SystemTimePoint now = SystemTimePoint::now();
    dsh.source_timestamp_sec_ = static_cast<ACE_INT32>(now.value().sec());
    dsh.source_timestamp_nanosec_ = now.value().usec() * 1000;
  }

  void
  write_control_msg(OpenDDS::DCPS::Message_Block_Ptr payload,
                    size_t size,
                    OpenDDS::DCPS::MessageId id,
                    OpenDDS::DCPS::SequenceNumber seq)
  {
    OpenDDS::DCPS::DataSampleHeader header;
    set_header_fields(header, size, GUID_UNKNOWN, seq, false, id);
    // no need to serialize header since rtps_udp transport ignores it
    send_control(header, OpenDDS::DCPS::move(payload));
  }

  void end_historic_samples(const RepoId& reader)
  {
    const void* pReader = static_cast<const void*>(&reader);
    OpenDDS::DCPS::Message_Block_Ptr mb(new ACE_Message_Block(DataSampleHeader::max_marshaled_size(),
                                                              ACE_Message_Block::MB_DATA,
                                                              new ACE_Message_Block(static_cast<const char*>(pReader),
                                                              sizeof(reader))));
    if (mb.get()) {
      mb->cont()->wr_ptr(sizeof(reader));
      // 'mb' would contain the DSHeader, but we skip it. mb.cont() has the data
      write_control_msg(move(mb), sizeof(reader), OpenDDS::DCPS::END_HISTORIC_SAMPLES,
                        OpenDDS::DCPS::SequenceNumber::SEQUENCENUMBER_UNKNOWN());
    }
  }

  void data_delivered(const DataSampleElement*)
  {
    ACE_DEBUG((LM_INFO, "SimpleDataWriter::data_delivered()\n"));
  }

  void control_delivered(const Message_Block_Ptr& sample)
  {
    ACE_UNUSED_ARG(sample);
    ACE_DEBUG((LM_DEBUG, "SimpleDataWriter::control_delivered()\n"));
  }

  void control_dropped(const Message_Block_Ptr& sample, bool dropped_by_transport)
  {
    ACE_UNUSED_ARG(sample);
    ACE_UNUSED_ARG(dropped_by_transport);
    ACE_DEBUG((LM_DEBUG, "SimpleDataWriter::control_dropped()\n"));
  }

  void notify_publication_disconnected(const ReaderIdSeq&) {}
  void notify_publication_reconnected(const ReaderIdSeq&) {}
  void notify_publication_lost(const ReaderIdSeq&) {}
  void remove_associations(const ReaderIdSeq&, bool) {}

  SendStateDataSampleList list_;
  DataSampleElement dsle_;
  ACE_Message_Block payload_;
  std::vector<ACE_Message_Block*> old_samples_;
};

struct TestParticipant: ACE_Event_Handler {
  TestParticipant(ACE_SOCK_Dgram& sock,
                  const OpenDDS::DCPS::GuidPrefix_t& prefix,
                  const OpenDDS::DCPS::EntityId_t& reader_ent)
    : sock_(sock), heartbeat_count_(0), acknack_count_(0), hbfrag_count_(0)
    , recv_hdr_(), recv_mb_(64 * 1024), reader_ent_(reader_ent)
  {
    const Header hdr = {
      {'R', 'T', 'P', 'S'}, PROTOCOLVERSION, VENDORID_OPENDDS,
      {INITIALIZE_GUID_PREFIX(prefix)}
    };
    std::memcpy(&hdr_, &hdr, sizeof(Header));
    for (CORBA::ULong i = 0; i < FRAG_SIZE; ++i) {
      data_for_frag_[i] = i % 256;
    }
    if (ACE_Reactor::instance()->register_handler(sock_.get_handle(),
                                                  this, READ_MASK) == -1) {
      ACE_ERROR((LM_ERROR, "ERROR in TestParticipant ctor, %p\n",
                 ACE_TEXT("register_handler")));
      throw std::exception();
    }
  }

  ~TestParticipant()
  {
    if (ACE_Reactor::instance()->remove_handler(sock_.get_handle(),
                                                ALL_EVENTS_MASK | DONT_CALL)
                                                == -1) {
      ACE_ERROR((LM_ERROR, "ERROR in TestParticipant dtor, %p\n",
                 ACE_TEXT("remove_handler")));
    }
  }

  bool send(const ACE_Message_Block& mb, const ACE_INET_Addr& send_to)
  {
    if (sock_.send(mb.rd_ptr(), mb.length(), send_to) < 0) {
      ACE_DEBUG((LM_DEBUG, "ERROR: in TestParticipant::send() %p\n",
                 ACE_TEXT("send")));
      return false;
    }
    return true;
  }

  bool send_data(const OpenDDS::DCPS::EntityId_t& writer,
                 const SequenceNumber_t& seq, const ACE_INET_Addr& send_to)
  {
#ifdef __SUNPRO_CC
    DataSubmessage ds = {
      {DATA, FLAG_E | FLAG_D, 0}, 0, DATA_OCTETS_TO_IQOS};
    ds.readerId = ENTITYID_UNKNOWN;
    ds.writerId = writer;
    ds.writerSN = seq;
#else
    const DataSubmessage ds = {
      {DATA, FLAG_E | FLAG_D, 0},
      0, DATA_OCTETS_TO_IQOS, ENTITYID_UNKNOWN, writer, seq, ParameterList()
    };
#endif
    size_t size = 0, padding = 0;
    gen_find_size(hdr_, size, padding);
    gen_find_size(ds, size, padding);
    size += 8; // CDR encap header + 4 bytes of data
    ACE_Message_Block mb(size + padding);
    Serializer ser(&mb, host_is_bigendian, Serializer::ALIGN_CDR);
    const ACE_CDR::ULong encap = 0x00000100, // {CDR_LE, options} in BE format
      data = 0xABCDABCD;
    bool ok = (ser << hdr_) && (ser << ds) && (ser << encap) && (ser << data);
    if (!ok) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize data\n"));
      return false;
    }
    return send(mb, send_to);
  }

  bool send_frag(const OpenDDS::DCPS::EntityId_t& writer, CORBA::ULong i,
                 const SequenceNumber_t& seq, const ACE_INET_Addr& send_to)
  {
    static const CORBA::ULong N = 3; // number of fragments to send
    ParameterList inlineQoS;
    if (i == 0) {
      inlineQoS.length(1);
      inlineQoS[0].string_data("my_topic_name");
      inlineQoS[0]._d(PID_TOPIC_NAME);
    }
#ifdef __SUNPRO_CC
    DataFragSubmessage df;
    df.smHeader.submessageId = DATA_FRAG;
    df.smHeader.flags = FLAG_E | (i ? 0 : FLAG_Q);
    df.smHeader.submessageLength = 0;
    df.extraFlags = 0;
    df.octetsToInlineQos = DATA_FRAG_OCTETS_TO_IQOS;
    df.readerId = ENTITYID_UNKNOWN;
    df.writerId = writer;
    df.writerSN = seq;
    df.fragmentStartingNum.value = i + 1;
    df.fragmentsInSubmessage = 1;
    df.fragmentSize = FRAG_SIZE;
    df.sampleSize = N * FRAG_SIZE;
    df.inlineQos = inlineQoS;
#else
    const DataFragSubmessage df = {
      {DATA_FRAG, CORBA::Octet(FLAG_E | (i ? 0 : FLAG_Q)), 0},
      0, DATA_FRAG_OCTETS_TO_IQOS, ENTITYID_UNKNOWN, writer, seq,
      {i + 1},       // fragmentStartingNum
      1,             // fragmentsInSubmessage
      FRAG_SIZE,     // fragmentSize (smallest fragmentSize allowed is 1KB)
      N * FRAG_SIZE, // sampleSize
      inlineQoS
    };
#endif
    size_t size = 0, padding = 0;
    gen_find_size(hdr_, size, padding);
    gen_find_size(df, size, padding);
    size += FRAG_SIZE;
    ACE_Message_Block mb(size + padding);
    Serializer ser(&mb, host_is_bigendian, Serializer::ALIGN_CDR);
    const ACE_CDR::ULong encap = 0x00000100; // {CDR_LE, options} in LE format
    bool ok = (ser << hdr_) && (ser << df);
    if (i == 0) ok &= (ser << encap);
    ok &= ser.write_octet_array(data_for_frag_,
                                i ? FRAG_SIZE : FRAG_SIZE - 4);
    if (!ok) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize data frag %d\n", i));
      return false;
    }
    return send(mb, send_to);
  }

  bool send_gap(const OpenDDS::DCPS::EntityId_t& writer,
                const SequenceNumber_t& seq, const ACE_INET_Addr& send_to)
  {
    LongSeq8 bitmap;
    bitmap.length(1);
    bitmap[0] = 0;
    SequenceNumber_t seq_pp = {seq.high, seq.low + 1};
    GapSubmessage gap = {
      {GAP, FLAG_E, 0}
#ifdef __SUNPRO_CC
    };
    gap.readerId = ENTITYID_UNKNOWN;
    gap.writerId = writer;
    gap.gapStart = seq;
    gap.gapList.bitmapBase = seq_pp;
    gap.gapList.numBits = 1;
    gap.gapList.bitmap = bitmap;
#else
    , ENTITYID_UNKNOWN, writer, seq,
      {seq_pp, 1, bitmap}
    };
#endif
    size_t size = 0, padding = 0;
    gen_find_size(hdr_, size, padding);
    gen_find_size(gap, size, padding);
    ACE_Message_Block mb(size + padding);
    Serializer ser(&mb, host_is_bigendian, Serializer::ALIGN_CDR);
    bool ok = (ser << hdr_) && (ser << gap);
    if (!ok) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize gap\n"));
      return false;
    }
    return send(mb, send_to);
  }

  bool send_hb(const OpenDDS::DCPS::EntityId_t& writer,
               const SequenceNumber_t& firstSN, const SequenceNumber_t& lastSN,
               const ACE_INET_Addr& send_to, const RepoId& reader = GUID_UNKNOWN)
  {
    const Message_Block_Ptr mb(buildHeartbeat(writer, hdr_,
                                              std::make_pair(firstSN, lastSN),
                                              heartbeat_count_, reader));
    if (!mb) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize heartbeat\n"));
      return false;
    }
    return send(*mb, send_to);
  }

  bool send_hbfrag(const OpenDDS::DCPS::EntityId_t& writer,
                   const SequenceNumber_t& seq, CORBA::ULong lastAvailFrag,
                   const ACE_INET_Addr& send_to)
  {
#ifdef __SUNPRO_CC
    HeartBeatFragSubmessage hbf;
    hbf.smHeader.submessageId = HEARTBEAT_FRAG;
    hbf.smHeader.flags = FLAG_E;
    hbf.smHeader.submessageLength = 0;
    hbf.readerId = ENTITYID_UNKNOWN;
    hbf.writerId = writer;
    hbf.writerSN = seq;
    hbf.lastFragmentNum.value = lastAvailFrag;
    hbf.count.value = ++hbfrag_count_;
#else
    const HeartBeatFragSubmessage hbf = {
      {HEARTBEAT_FRAG, FLAG_E, 0},
      ENTITYID_UNKNOWN, writer, seq, {lastAvailFrag}, {++hbfrag_count_}
    };
#endif
    size_t size = 0, padding = 0;
    gen_find_size(hdr_, size, padding);
    gen_find_size(hbf, size, padding);
    ACE_Message_Block mb(size + padding);
    Serializer ser(&mb, host_is_bigendian, Serializer::ALIGN_CDR);
    bool ok = (ser << hdr_) && (ser << hbf);
    if (!ok) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize heartbeatfrag\n"));
      return false;
    }
    return send(mb, send_to);
  }

  bool send_simple_an(const OpenDDS::DCPS::EntityId_t& writer,
                      const SequenceNumber_t& nack, const ACE_INET_Addr& send_to,
                      bool set_bit_in_bitmap = true)
  {
    LongSeq8 bitmap;
    bitmap.length(1);
    bitmap[0] = set_bit_in_bitmap ? 0xF0000000 : 0;
#ifdef __SUNPRO_CC
    AckNackSubmessage an;
    an.smHeader.submessageId = ACKNACK;
    an.smHeader.flags = FLAG_E;
    an.smHeader.submessageLength = 0;
    an.readerId = reader_ent_;
    an.writerId = writer;
    an.readerSNState.bitmapBase = nack;
    an.readerSNState.numBits = 1;
    an.readerSNState.bitmap = bitmap;
    an.count.value = ++acknack_count_;
#else
    const AckNackSubmessage an = {
      {ACKNACK, FLAG_E, 0},
      reader_ent_, writer,
      {nack, 1, bitmap},
      {++acknack_count_}
    };
#endif
    size_t size = 0, padding = 0;
    gen_find_size(hdr_, size, padding);
    gen_find_size(an, size, padding);
    ACE_Message_Block mb(size + padding);
    Serializer ser(&mb, host_is_bigendian, Serializer::ALIGN_CDR);
    bool ok = (ser << hdr_) && (ser << an);
    if (!ok) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize acknack\n"));
      return false;
    }
    return send(mb, send_to);
  }

  bool send_full_an(const OpenDDS::DCPS::EntityId_t& writer, SequenceNumber low, SequenceNumber high, const ACE_INET_Addr& send_to)
  {
    DisjointSequence temp_recvd = recvd_;
    temp_recvd.insert(SequenceRange(0, low.previous()));
    if (high > temp_recvd.cumulative_ack()) {
      temp_recvd.insert(++high);
    }

    CORBA::ULong numBits = 0;
    LongSeq8 bitmap;
    bitmap.length(8);
    SequenceNumber base_sn = recvd_.empty() ? SequenceNumber() : ++(temp_recvd.cumulative_ack());
    SequenceNumber_t base = {base_sn.getHigh(), base_sn.getLow()};
    if (!temp_recvd.to_bitmap(&bitmap[0], 8, numBits, true)) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to populate bitmap\n"));
      return false;
    }
#ifdef __SUNPRO_CC
    AckNackSubmessage an;
    an.smHeader.submessageId = ACKNACK;
    an.smHeader.flags = FLAG_E | (numBits ? 0 : FLAG_F);
    an.smHeader.submessageLength = 0;
    an.readerId = reader_ent_;
    an.writerId = writer;
    an.readerSNState.bitmapBase = base;
    an.readerSNState.numBits = numBits;
    an.readerSNState.bitmap = bitmap;
    an.count.value = ++acknack_count_;
#else
    const AckNackSubmessage an = {
      {ACKNACK, static_cast<CORBA::Octet>(FLAG_E | (numBits ? 0 : FLAG_F)), 0},
      reader_ent_, writer,
      {base, numBits, bitmap},
      {++acknack_count_}
    };
#endif
    size_t size = 0, padding = 0;
    gen_find_size(hdr_, size, padding);
    gen_find_size(an, size, padding);
    ACE_Message_Block mb(size + padding);
    Serializer ser(&mb, host_is_bigendian, Serializer::ALIGN_CDR);
    bool ok = (ser << hdr_) && (ser << an);
    if (!ok) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize acknack\n"));
      return false;
    }
    return send(mb, send_to);
  }

  void set_ignore_count(const SequenceNumber& sn, int64_t count) {
    ignore_counts_[sn] = count;
  }

  int handle_input(ACE_HANDLE)
  {
    ACE_INET_Addr peer;
    recv_mb_.reset();
    ssize_t ret = sock_.recv(recv_mb_.wr_ptr(), recv_mb_.space(), peer);
    if (ret < 0) {
      ACE_DEBUG((LM_DEBUG, "ERROR: in handle_input() %p\n", ACE_TEXT("recv")));
      return -1;
    } else if (ret == 0) {
      return -1;
    }
    recv_mb_.wr_ptr(ret);
    Serializer ser(&recv_mb_, host_is_bigendian, Serializer::ALIGN_CDR);
    if (!(ser >> recv_hdr_)) {
      ACE_ERROR((LM_ERROR,
        "ERROR: in handle_input() failed to deserialize RTPS Header\n"));
      return -1;
    }
    while (recv_mb_.length() > 3) {
      char subm = recv_mb_.rd_ptr()[0], flags = recv_mb_.rd_ptr()[1];
      ser.swap_bytes((flags & FLAG_E) != ACE_CDR_BYTE_ORDER);
      switch (subm) {
      case ACKNACK:
        if (!recv_an(ser, peer)) return false;
        break;
      case GAP:
        if (!recv_gap(ser, peer)) return false;
        break;
      case DATA:
        if (!recv_data(ser, peer)) return false;
        break;
      case HEARTBEAT:
        if (!recv_hb(ser, peer)) return false;
        break;
      case NACK_FRAG:
        if (!recv_nackfrag(ser, peer)) return false;
        break;
      default:
#ifdef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
        ACE_DEBUG((LM_INFO, "Received submessage type: %u\n", unsigned(subm)));
#else
        if (static_cast<size_t>(subm) < gen_OpenDDS_RTPS_SubmessageKind_names_size) {
          ACE_DEBUG((LM_INFO, "Received submessage type: %C\n",
                     gen_OpenDDS_RTPS_SubmessageKind_names[static_cast<size_t>(subm)]));
        } else {
          ACE_ERROR((LM_ERROR, "ERROR: Received unknown submessage type: %u\n",
                     unsigned(subm)));
        }
#endif
        SubmessageHeader smh;
        if (!(ser >> smh)) {
          ACE_ERROR((LM_ERROR, "ERROR: in handle_input() failed to deserialize "
                               "SubmessageHeader\n"));
          return -1;
        }
        if (smh.submessageLength == 0) {
          return 0;
        }
        recv_mb_.rd_ptr(smh.submessageLength);
      }
    }
    return 0;
  }

  bool recv_an(Serializer& ser, const ACE_INET_Addr& peer)
  {
    AckNackSubmessage an;
    if (!(ser >> an)) {
      ACE_ERROR((LM_ERROR,
        "ERROR: recv_an() failed to deserialize AckNackSubmessage\n"));
      return false;
    }
    bool nack = false;
    for (CORBA::ULong i = 0; i < an.readerSNState.numBits; ++i) {
      if (an.readerSNState.bitmap[i / 32] & (1 << (31 - (i % 32)))) {
        nack = true;
        SequenceNumber_t seq = an.readerSNState.bitmapBase;
        seq.low += i;
        if (seq.low == 4) {
          ACE_DEBUG((LM_INFO, "recv_an() gap retransmit %d\n", seq.low));
          if (!send_gap(an.writerId, seq, peer)) {
            return false;
          }
        } else {
          ACE_DEBUG((LM_INFO, "recv_an() data retransmit %d\n", seq.low));
          if (!send_data(an.writerId, seq, peer)) {
            return false;
          }
        }
      }
    }
    if (!nack) {
      ACE_DEBUG((LM_DEBUG, "recv_an() no retransmission requested\n"));
    }
    return true;
  }

  bool recv_nackfrag(Serializer& ser, const ACE_INET_Addr& peer)
  {
    NackFragSubmessage nf;
    if (!(ser >> nf)) {
      ACE_ERROR((LM_ERROR,
        "ERROR: recv_nackfrag() failed to deserialize NackFragSubmessage\n"));
      return false;
    }
    if (nf.writerSN.low != 6 && nf.writerSN.low != 7) {
      ACE_ERROR((LM_ERROR,
                 "ERROR: recv_nackfrag() unexpected NACK_FRAG seq %d\n",
                 nf.writerSN.low));
      return true;
    }
    for (CORBA::ULong i = 0; i < nf.fragmentNumberState.numBits; ++i) {
      if (nf.fragmentNumberState.bitmap[i / 32] & (1 << (31 - (i % 32)))) {
        FragmentNumber_t frag = nf.fragmentNumberState.bitmapBase;
        frag.value += i;
        if (nf.writerSN.low == 6 && frag.value != 2) {
          ACE_ERROR((LM_ERROR,
                     "ERROR: recv_nackfrag() unexpected NACK_FRAG frag %d\n",
                     frag.value));
          return true;
        }
        if (nf.writerSN.low == 6 || frag.value == 2) {
          ACE_DEBUG((LM_INFO, "recv_nackfrag() retransmit %d:%d\n",
                     nf.writerSN.low, frag.value));
          if (!send_frag(nf.writerId, frag.value - 1, nf.writerSN, peer)) {
            return false;
          }
        }
      }
    }
    return true;
  }

  bool recv_gap(Serializer& ser, const ACE_INET_Addr&)
  {
    GapSubmessage gap;
    if (!(ser >> gap)) {
      ACE_ERROR((LM_ERROR,
        "ERROR: recv_gap() failed to deserialize GapSubmessage\n"));
      return false;
    }
    ACE_DEBUG((LM_INFO, "recv_gap() gapStart = %d gapListBase = %d\n",
               gap.gapStart.low, gap.gapList.bitmapBase.low));
    recvd_.insert(SequenceRange(gap.gapStart.low,
                                gap.gapList.bitmapBase.low - 1));
    return true;
  }

  bool recv_data(Serializer& ser, const ACE_INET_Addr&)
  {
    DataSubmessage data;
    if (!(ser >> data)) {
      ACE_ERROR((LM_ERROR,
        "ERROR: recv_data() failed to deserialize DataSubmessage\n"));
      return false;
    }
    ACE_DEBUG((LM_INFO, "recv_data() seq = %d\n", data.writerSN.low));
    if (data.smHeader.submessageLength) {
      ser.skip(data.smHeader.submessageLength - 20);
      // 20 == size of Data headers after smHeader (assuming no Inline QoS)
    } else {
      ser.skip(8);  // our data payloads are 8 bytes
    }
    if (--ignore_counts_[makeSN(data.writerSN.high, data.writerSN.low)] < 0) {
      ACE_DEBUG((LM_DEBUG, "recv_data - inserting data SN %q into recvd_\n", makeSN(data.writerSN.high, data.writerSN.low).getValue()));
      recvd_.insert(data.writerSN.low);
      recvd_data_.insert(data.writerSN.low);
    } else {
      ACE_DEBUG((LM_DEBUG, "recv_data - ignoring data SN %q\n", makeSN(data.writerSN.high, data.writerSN.low).getValue()));
    }
    return true;
  }

  bool recv_hb(Serializer& ser, const ACE_INET_Addr& peer)
  {
    HeartBeatSubmessage hb;
    if (!(ser >> hb)) {
      ACE_ERROR((LM_ERROR,
        "ERROR: recv_hb() failed to deserialize HeartBeatSubmessage\n"));
      return false;
    }
    ACE_DEBUG((LM_INFO, "recv_hb() first = %d last = %d\n",
               hb.firstSN.low, hb.lastSN.low));
    const bool flag_f = hb.smHeader.flags & 2;
    if (!flag_f && hb.lastSN.low == 0) {
      const SequenceNumber_t zero = {0, 0};
      if (!send_simple_an(hb.writerId, zero, peer, false)) {
        return false;
      }
    } else if (!flag_f && hb.firstSN.low == 1 && hb.lastSN.low == 1) {
      const SequenceNumber_t one = {0, 1};
      if (!send_simple_an(hb.writerId, one, peer, false)) {
        return false;
      }
    }
    return send_full_an(hb.writerId, makeSN(hb.firstSN.high, hb.firstSN.low), makeSN(hb.lastSN.high, hb.lastSN.low), peer);
  }

  ACE_SOCK_Dgram& sock_;
  CORBA::Long heartbeat_count_, acknack_count_, hbfrag_count_;
  Header hdr_, recv_hdr_;
  ACE_Message_Block recv_mb_;
  OpenDDS::DCPS::EntityId_t reader_ent_;
  DisjointSequence recvd_;
  DisjointSequence recvd_data_;
  OPENDDS_MAP(SequenceNumber, CORBA::Long) ignore_counts_;
  static const ACE_CDR::UShort FRAG_SIZE = 1024;
  ACE_CDR::Octet data_for_frag_[FRAG_SIZE];
};


void reactor_wait()
{
  ACE_Time_Value one(1);
  ACE_Reactor::instance()->run_reactor_event_loop(one);
}

struct ReactorTask : ACE_Task_Base {

  ReactorTask()
  {
    activate();
  }

  int svc()
  {
    ACE_Reactor* reactor = ACE_Reactor::instance();
    ACE_thread_t old_owner;
    reactor->owner(ACE_Thread_Manager::instance()->thr_self(), &old_owner);
    reactor_wait();
    reactor->owner(old_owner);
    return 0;
  }
};

void transport_setup()
{
  TransportInst_rch inst =
    TheTransportRegistry->create_inst("my_rtps", "rtps_udp");
  RtpsUdpInst* rtps_inst = dynamic_cast<RtpsUdpInst*>(inst.in());
  if (!rtps_inst) {
    std::cerr << "ERROR: Could not cast to RtpsUdpInst\n";
    return;
  }
  rtps_inst->use_multicast_ = false;
  rtps_inst->datalink_release_delay_ = 0;
  rtps_inst->heartbeat_period_ = TimeDuration::from_msec(500);
  TransportConfig_rch cfg = TheTransportRegistry->create_config("cfg");
  cfg->instances_.push_back(inst);
  TheTransportRegistry->global_config(cfg);
}

void make_guids(OpenDDS::DCPS::GUID_t& writer1, OpenDDS::DCPS::GUID_t& reader1,
                OpenDDS::DCPS::GUID_t& writer2, OpenDDS::DCPS::GUID_t& reader2)
{
  GuidGenerator gen;
  gen.populate(writer1);
  OpenDDS::DCPS::EntityId_t entid = {
    {0, 1, 2}, ENTITYKIND_USER_WRITER_WITH_KEY
  };
  writer1.entityId = entid;

  reader1 = writer1;
  reader1.entityId.entityKey[2] = 3;
  reader1.entityId.entityKind = ENTITYKIND_USER_READER_WITH_KEY;

  gen.populate(writer2);
  writer2.entityId = entid;
  writer2.entityId.entityKey[2] = 4;

  reader2 = writer2;
  reader2.entityId.entityKey[2] = 5;
  reader2.entityId.entityKind = ENTITYKIND_USER_READER_WITH_KEY;
}

void make_blob(const ACE_INET_Addr& part1_addr, ACE_Message_Block& mb_locator)
{
  LocatorSeq part1_locators;
  part1_locators.length(1);
  part1_locators[0].kind =
#ifdef ACE_HAS_IPV6
    (part1_addr.get_type() == AF_INET6) ? LOCATOR_KIND_UDPv6 :
#endif
      LOCATOR_KIND_UDPv4;
  part1_locators[0].port = part1_addr.get_port_number();
  address_to_bytes(part1_locators[0].address, part1_addr);
  size_t size_locator = 0, padding_locator = 0;
  gen_find_size(part1_locators, size_locator, padding_locator);
  mb_locator.init(size_locator + padding_locator + 1);
  Serializer ser_loc(&mb_locator, ACE_CDR_BYTE_ORDER, Serializer::ALIGN_CDR);
  ser_loc << part1_locators;
  ser_loc << ACE_OutputCDR::from_boolean(false); // requires inline QoS
}

bool blob_to_addr(const TransportBLOB& blob, ACE_INET_Addr& addr)
{
  ACE_Data_Block db(blob.length(), ACE_Message_Block::MB_DATA,
    reinterpret_cast<const char*>(blob.get_buffer()),
    0 /*alloc*/, 0 /*lock*/, ACE_Message_Block::DONT_DELETE, 0 /*db_alloc*/);
  ACE_Message_Block mb(&db, ACE_Message_Block::DONT_DELETE, 0 /*mb_alloc*/);
  mb.wr_ptr(mb.space());
  Serializer ser(&mb, ACE_CDR_BYTE_ORDER, Serializer::ALIGN_CDR);
  LocatorSeq locators;
  if (!(ser >> locators) || locators.length() < 1) {
    ACE_DEBUG((LM_DEBUG,
               "ERROR: couldn't deserialize Locators from participant 2\n"));
    return false;
  }
  if (locators[0].kind == LOCATOR_KIND_UDPv6) {
#ifdef ACE_HAS_IPV6
    addr.set_type(AF_INET6);
#endif
    addr.set_address(reinterpret_cast<const char*>(locators[0].address), 16, 0 /*encode*/);
  } else if (locators[0].kind == LOCATOR_KIND_UDPv4) {
    addr.set_type(AF_INET);
    addr.set_address(reinterpret_cast<const char*>(locators[0].address) + 12,
                     4, 0 /*network order*/);
  } else {
    ACE_DEBUG((LM_DEBUG, "ERROR: unknown locator kind\n"));
    return false;
  }
  addr.set_port_number(locators[0].port);
  return true;
}

bool run_test()
{
  transport_setup();

  // Set up GUIDs for 2 readers and 2 writers.
  // Each pair of reader+writer belongs to the one participant (GuidPrefix_t):
  // Participant 1 contains writer1 and reader1 and will use sockets directly
  // Participant 2 contains writer2 and reader2 and will use the OpenDDS tport
  // Associations: writer1 <-> reader2 and writer2 <-> reader1
  OpenDDS::DCPS::GUID_t
    writer1(GUID_UNKNOWN), reader1(GUID_UNKNOWN),
    writer2(GUID_UNKNOWN), reader2(GUID_UNKNOWN);
  make_guids(writer1, reader1, writer2, reader2);

  ACE_SOCK_Dgram part1_sock;
  ACE_INET_Addr part1_addr;
  if (!open_appropriate_socket_type(part1_sock, part1_addr)) {
    std::cerr << "ERROR: run_test() unable to open part1_sock" << std::endl;
    exit(1);
  }
  part1_sock.get_local_addr(part1_addr);
#ifdef OPENDDS_SAFETY_PROFILE
  part1_addr.set(part1_addr.get_port_number(), "127.0.0.1");
#else
  part1_addr.set(part1_addr.get_port_number(), "localhost");
#endif
  SimpleDataWriter sdw2(writer2);
  sdw2.enable_transport(true /*reliable*/, true /*durable*/);

  SimpleDataReader sdr2(reader2);
  sdr2.enable_transport(true /*reliable*/, true /*durable*/);


  // "local" setup is now done, start making associations
  ACE_Message_Block mb_locator;
  make_blob(part1_addr, mb_locator);

  AssociationData part1_writer;
  part1_writer.remote_id_ = writer1;
  part1_writer.remote_reliable_ = true;
  part1_writer.remote_durable_ = true;
  part1_writer.remote_data_.length(1);
  part1_writer.remote_data_[0].transport_type = "rtps_udp";
  message_block_to_sequence(mb_locator, part1_writer.remote_data_[0].data);
  if (!sdr2.associate(part1_writer, false /*active*/)) {
    ACE_DEBUG((LM_DEBUG,
               "SimpleDataReader(reader2) could not associate with writer1\n"));
    return false;
  }

  const TransportLocatorSeq& part2_loc = sdr2.connection_info();
  if (part2_loc.length() < 1) {
    ACE_DEBUG((LM_DEBUG,
               "ERROR: couldn't get connection_info() for participant 2\n"));
    return false;
  }

  ACE_INET_Addr part2_addr;
  if (!blob_to_addr(part2_loc[0].data, part2_addr)) {
    return false;
  }
#if defined (ACE_HAS_IPV6)
  ACE_INET_Addr tmp;
  part1_sock.get_local_addr(tmp);
  if (tmp.get_type() == AF_INET6 && part2_addr.get_type() == AF_INET) {
    //need to map address to IPV6
    LocatorSeq locators;
    locators.length(1);
    locators[0].kind = address_to_kind(part2_addr);
    locators[0].port = part2_addr.get_port_number();
    address_to_bytes(locators[0].address, part2_addr);
    locator_to_address(part2_addr, locators[0], tmp.get_type() != AF_INET);
  }
#endif

  // Associations are done, now test the real DR (SimpleDataReader) using our
  // TestParticipant class to interact with it over the socket directly.
  ACE_DEBUG((LM_INFO, ">>> Starting test of DataReader\n"));

  TestParticipant part1(part1_sock, reader1.guidPrefix, reader1.entityId);
  part1.set_ignore_count(makeSN(0, 1), 1);
  SequenceNumber_t first_seq = {0, 1}, seq = first_seq;
  if (!part1.send_hb(writer1.entityId, seq, seq, part2_addr, reader2)) {
    return false;
  }

  if (!part1.send_data(writer1.entityId, seq, part2_addr)) {
    return false;
  }

  reactor_wait();

  // this heartbeat isn't final, so reader needs to ack even if it has all data:
  if (!part1.send_hb(writer1.entityId, seq, seq, part2_addr)) {
    return false;
  }
  reactor_wait();
  sdr2.wait_for_assoc(writer1);

  seq.low = 3; // #2 is the "lost" message
  if (!part1.send_data(writer1.entityId, seq, part2_addr)) {
    return false;
  }
  if (!part1.send_hb(writer1.entityId, first_seq, seq, part2_addr)) {
    return false;
  }
  seq.low = 5; // #4 will be a GAP when the reader requests it via a nack
  if (!part1.send_data(writer1.entityId, seq, part2_addr)) {
    return false;
  }
  if (!part1.send_hb(writer1.entityId, first_seq, seq, part2_addr)) {
    return false;
  }

  reactor_wait();
  // further heartbeats should not generate any negative acks
  if (!part1.send_hb(writer1.entityId, first_seq, seq, part2_addr)) {
    return false;
  }
  reactor_wait();

  SequenceNumber_t seq_frag = {0, 6};
  if (!part1.send_frag(writer1.entityId, 0, seq_frag, part2_addr)) {
    return false;
  }
  if (!part1.send_frag(writer1.entityId, 2, seq_frag, part2_addr)) {
    return false;
  }
  reactor_wait();
  seq.low = 5; // can't heartbeat until all frags sent
  if (!part1.send_hb(writer1.entityId, first_seq, seq, part2_addr)) {
    return false;
  }
  if (!part1.send_hbfrag(writer1.entityId, seq_frag, 2, part2_addr)) {
    return false;
  }
  reactor_wait(); // reader replies with its NACK_FRAG, part1 sends frag

  seq = seq_frag; // 6 is now fully sent
  if (!part1.send_hb(writer1.entityId, first_seq, seq, part2_addr)) {
    return false;
  }
  reactor_wait();

  seq_frag.low = 7; // send HBFrag before actually sending the fragment
  if (!part1.send_hbfrag(writer1.entityId, seq_frag, 1, part2_addr)) {
    return false;
  }
  reactor_wait();
  if (!part1.send_frag(writer1.entityId, 0, seq_frag, part2_addr)) {
    return false;
  }
  seq.low = 8;
  if (!part1.send_data(writer1.entityId, seq, part2_addr)) {
    return false;
  }
  SequenceNumber_t seq_hb = {0, 6};
  if (!part1.send_hb(writer1.entityId, first_seq, seq_hb, part2_addr)) {
    return false;
  }
  reactor_wait(); // HB 6 with missing frag 7 should result in NF

  if (!part1.send_hbfrag(writer1.entityId, seq_frag, 2, part2_addr)) {
    return false;
  }
  reactor_wait(); // HF should also result in NF
  if (!part1.send_frag(writer1.entityId, 2, seq_frag, part2_addr)) {
    return false;
  }
  if (!part1.send_hb(writer1.entityId, first_seq, seq, part2_addr)) {
    return false;
  }
  reactor_wait();

  SequenceNumber sn;
  sn.setValue(seq.high, seq.low);
  if (sdr2.recvd_.disjoint() || sdr2.recvd_.empty()
      || sdr2.recvd_.high() != sn || sdr2.recvd_.low() != SequenceNumber()) {
    ACE_ERROR((LM_ERROR, "ERROR: reader2 did not receive expected data\n"));
  }


  // Use the real DDS DataWriter (sdw2) against the test reader
  ACE_DEBUG((LM_INFO, ">>> Starting test of DataWriter\n"));
  AssociationData part1_reader = part1_writer;
  part1_reader.remote_id_ = reader1;
  part1_reader.remote_durable_ = true;
  ::ReactorTask rt;
  if (!sdw2.associate(part1_reader, true /*active*/)) {
    ACE_DEBUG((LM_DEBUG,
               "SimpleDataWriter(writer2) could not associate with reader1\n"));
    sdr2.disassociate(writer1);
    return false;
  }
  sdw2.wait_for_assoc(reader1);
  rt.wait();

  SequenceNumber seq_dw2;
  sdw2.send_data(seq_dw2++, reader1);  // send #1 - #2, test reader will nack #1
  sdw2.send_data(seq_dw2++, reader1);
  reactor_wait();
  sdw2.end_historic_samples(reader1);
  reactor_wait();

  if (part1.recvd_.disjoint()) {
    ACE_ERROR((LM_ERROR, "ERROR: reader1 did not receive expected data (disjoint)\n"));
    return false;
  }

  if (part1.recvd_.empty()) {
    ACE_ERROR((LM_ERROR, "ERROR: reader1 did not receive expected data (empty)\n"));
    return false;
  }

  if (part1.recvd_.high() != seq_dw2.previous()) {
    ACE_ERROR((LM_ERROR, "ERROR: reader1 did not receive expected data and/or gap (high)\n"));
    return false;
  }

  if (part1.recvd_.low() != SequenceNumber()) {
    ACE_ERROR((LM_ERROR, "ERROR: reader1 did not receive expected data and/or gap (low)\n"));
    return false;
  }

  if (part1.recvd_data_.high() != seq_dw2.previous()) {
    ACE_ERROR((LM_ERROR, "ERROR: reader1 did not receive expected data (high)\n"));
    return false;
  }

  if (part1.recvd_data_.low() != SequenceNumber()) {
    ACE_ERROR((LM_ERROR, "ERROR: reader1 did not receive expected data (low)\n"));
    return false;
  }

  // cleanup
  sdw2.disassociate(reader1);
  sdr2.disassociate(writer1);
  sdw2.transport_stop();
  sdr2.transport_stop();
  return true;
}

int ACE_TMAIN(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
  try
  {
    ::DDS::DomainParticipantFactory_var dpf =
      TheServiceParticipant->get_domain_participant_factory();
  }
  catch (const CORBA::BAD_PARAM& ex)
  {
    ex._tao_print_exception("Exception caught in rtps_reliability.cpp:");
    return 1;
  }

  bool ok = false;
  try {
    ok = run_test();
    if (!ok) {
      ACE_ERROR((LM_ERROR, "ERROR: test failed\n"));
    }
  } catch (const OpenDDS::DCPS::Transport::Exception& e) {
    ACE_ERROR((LM_ERROR, "EXCEPTION: %C\n", typeid(e).name()));
  } catch (const CORBA::Exception& e) {
    ACE_ERROR((LM_ERROR, "EXCEPTION: %C\n", e._info().c_str()));
  } catch (const std::exception& e) {
    ACE_ERROR((LM_ERROR, "EXCEPTION: %C\n", e.what()));
  } catch (...) {
    ACE_ERROR((LM_ERROR, "Unknown EXCEPTION\n"));
  }
  TheServiceParticipant->shutdown();
  ACE_Thread_Manager::instance()->wait();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
