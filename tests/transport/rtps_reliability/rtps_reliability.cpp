// Test for the Reliability features (Heartbeat, AckNack, Gap) in the RtpsUdp
// OpenDDS transport implementation.

#include "dds/DCPS/transport/rtps_udp/RtpsUdpInst.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/framework/TransportClient.h"
#include "dds/DCPS/transport/framework/TransportExceptions.h"

#include "dds/DCPS/RTPS/RtpsMessageTypesTypeSupportImpl.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/MessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include "dds/DCPS/RTPS/GuidGenerator.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/AssociationData.h"

#include <tao/Exception.h>

#include <ace/OS_main.h>
#include <ace/Thread_Manager.h>
#include <ace/Reactor.h>

#include <cstdlib>
#include <typeinfo>
#include <exception>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

const bool host_is_bigendian = !ACE_CDR_BYTE_ORDER;
const char* smkinds[] = {"RESERVED_0", "PAD", "RESERVED_2", "RESERVED_3",
  "RESERVED_4", "RESERVED_5", "ACKNACK", "HEARTBEAT", "GAP", "INFO_TS",
  "RESERVED_10", "RESERVED_11", "INFO_SRC", "INFO_REPLY_IP4", "INFO_DST",
  "INFO_REPLY", "RESERVED_16", "RESERVED_17", "NACK_FRAG", "HEARTBEAT_FRAG",
  "RESERVED_20", "DATA", "DATA_FRAG"};
const size_t n_smkinds = sizeof(smkinds) / sizeof(smkinds[0]);


struct SimpleTC: TransportClient {
  explicit SimpleTC(const RepoId& local) : local_id_(local) {}

  using TransportClient::enable_transport;
  using TransportClient::associate;
  using TransportClient::disassociate;
  using TransportClient::send;
  using TransportClient::connection_info;

  const RepoId& get_repo_id() const { return local_id_; }
  bool check_transport_qos(const TransportInst&) { return true; }
  CORBA::Long get_priority_value(const AssociationData&) const { return 0; }

  RepoId local_id_;
};


struct SimpleDataReader: SimpleTC, TransportReceiveListener {
  explicit SimpleDataReader(const RepoId& sub_id)
    : SimpleTC(sub_id), have_frag_(false) {}

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
        ACE_DEBUG((LM_ERROR, "ERROR: unexpected reassembled sample length\n"));
      }
      if (have_frag_) {
        ACE_DEBUG((LM_ERROR, "ERROR: duplicate delivery from DATA_FRAG\n"));
      }
      have_frag_ = true;
    }
  }

  void notify_subscription_disconnected(const WriterIdSeq&) {}
  void notify_subscription_reconnected(const WriterIdSeq&) {}
  void notify_subscription_lost(const WriterIdSeq&) {}
  void notify_connection_deleted() {}
  void remove_associations(const WriterIdSeq&, bool) {}

  DisjointSequence recvd_;
  bool have_frag_;
};


struct SimpleDataWriter: SimpleTC, TransportSendListener {
  explicit SimpleDataWriter(const RepoId& pub_id)
    : SimpleTC(pub_id)
    , alloc_(2, sizeof(TransportSendElementAllocator))
    , dsle_(pub_id, this, 0, &alloc_, 0)
  {
    list_.head_ = list_.tail_ = &dsle_;
    list_.size_ = 1;
    dsle_.header_.message_id_ = SAMPLE_DATA;
    dsle_.header_.message_length_ = 8;
    dsle_.header_.byte_order_ = ACE_CDR_BYTE_ORDER;
    payload_.init(dsle_.header_.message_length_);
    const ACE_CDR::ULong encap = 0x00000100, // {CDR_LE, options} in BE format
      data = 0xDCBADCBA;
    Serializer ser(&payload_, host_is_bigendian, Serializer::ALIGN_CDR);
    ser << encap;
    ser << data;
  }

  void send_data(const SequenceNumber& seq)
  {
    dsle_.header_.sequence_ = seq;
    dsle_.sample_ =
      new ACE_Message_Block(DataSampleHeader::max_marshaled_size());
    *dsle_.sample_ << dsle_.header_;
    dsle_.sample_->cont(payload_.duplicate());
    ACE_DEBUG((LM_INFO, "sending with seq#: %q\n", seq.getValue()));
    send(list_);
  }

  void data_delivered(const DataSampleListElement*)
  {
    ACE_DEBUG((LM_INFO, "SimpleDataWriter::data_delivered()\n"));
  }

  void notify_publication_disconnected(const ReaderIdSeq&) {}
  void notify_publication_reconnected(const ReaderIdSeq&) {}
  void notify_publication_lost(const ReaderIdSeq&) {}
  void notify_connection_deleted() {}
  void remove_associations(const ReaderIdSeq&, bool) {}

  TransportSendElementAllocator alloc_;
  DataSampleList list_;
  DataSampleListElement dsle_;
  ACE_Message_Block payload_;
};


enum RtpsFlags { FLAG_E = 1, FLAG_Q = 2, FLAG_D = 4 };

struct TestParticipant: ACE_Event_Handler {
  TestParticipant(ACE_SOCK_Dgram& sock,
                  const OpenDDS::DCPS::GuidPrefix_t& prefix,
                  const OpenDDS::DCPS::EntityId_t& reader_ent)
    : sock_(sock), heartbeat_count_(0), acknack_count_(0), hbfrag_count_(0)
    , recv_mb_(64 * 1024), do_nack_(true), reader_ent_(reader_ent)
  {
    const Header hdr = {
      {'R', 'T', 'P', 'S'}, PROTOCOLVERSION, VENDORID_OPENDDS,
      {prefix[0], prefix[1], prefix[2], prefix[3], prefix[4], prefix[5],
       prefix[6], prefix[7], prefix[8], prefix[9], prefix[10], prefix[11]}
    };
    hdr_ = hdr;
    for (CORBA::ULong i = 0; i < FRAG_SIZE; ++i) {
      data_for_frag_[i] = i % 256;
    }
    if (ACE_Reactor::instance()->register_handler(sock_.get_handle(),
                                                  this, READ_MASK) == -1) {
      ACE_DEBUG((LM_ERROR, "ERROR in TestParticipant ctor, %p\n",
                 ACE_TEXT("register_handler")));
      throw std::exception();
    }
  }

  ~TestParticipant()
  {
    if (ACE_Reactor::instance()->remove_handler(sock_.get_handle(),
                                                ALL_EVENTS_MASK | DONT_CALL)
                                                == -1) {
      ACE_DEBUG((LM_ERROR, "ERROR in TestParticipant dtor, %p\n",
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
    const DataSubmessage ds = {
      {DATA, FLAG_E | FLAG_D, 0},
      0, DATA_OCTETS_TO_IQOS, ENTITYID_UNKNOWN, writer, seq, ParameterList()
    };
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
    DataFragSubmessage df = {
      {DATA_FRAG, FLAG_E | (i ? 0 : FLAG_Q), 0},
      0, DATA_FRAG_OCTETS_TO_IQOS, ENTITYID_UNKNOWN, writer, seq,
      {i + 1},       // fragmentStartingNum
      1,             // fragmentsInSubmessage
      FRAG_SIZE,     // fragmentSize (smallest fragmentSize allowed is 1KB)
      N * FRAG_SIZE, // sampleSize
      inlineQoS
    };
    size_t size = 0, padding = 0;
    gen_find_size(hdr_, size, padding);
    gen_find_size(df, size, padding);
    size += FRAG_SIZE;
    ACE_Message_Block mb(size + padding);
    Serializer ser(&mb, host_is_bigendian, Serializer::ALIGN_CDR);
    const ACE_CDR::ULong encap = 0x00000100; // {CDR_LE, options} in BE format
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
    const GapSubmessage gap = {
      {GAP, FLAG_E, 0},
      ENTITYID_UNKNOWN, writer, seq,
      {seq_pp, 1, bitmap}
    };
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
               const ACE_INET_Addr& send_to)
  {
    const HeartBeatSubmessage hb = {
      {HEARTBEAT, FLAG_E, 0},
      ENTITYID_UNKNOWN, writer, firstSN, lastSN, {++heartbeat_count_}
    };
    size_t size = 0, padding = 0;
    gen_find_size(hdr_, size, padding);
    gen_find_size(hb, size, padding);
    ACE_Message_Block mb(size + padding);
    Serializer ser(&mb, host_is_bigendian, Serializer::ALIGN_CDR);
    bool ok = (ser << hdr_) && (ser << hb);
    if (!ok) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize heartbeat\n"));
      return false;
    }
    return send(mb, send_to);
  }

  bool send_hbfrag(const OpenDDS::DCPS::EntityId_t& writer,
                   const SequenceNumber_t& seq, CORBA::ULong lastAvailFrag,
                   const ACE_INET_Addr& send_to)
  {
    const HeartBeatFragSubmessage hbf = {
      {HEARTBEAT_FRAG, FLAG_E, 0},
      ENTITYID_UNKNOWN, writer, seq, {lastAvailFrag}, {++hbfrag_count_}
    };
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

  bool send_an(const OpenDDS::DCPS::EntityId_t& writer,
               const SequenceNumber_t& nack, const ACE_INET_Addr& send_to)
  {
    LongSeq8 bitmap;
    bitmap.length(1);
    bitmap[0] = 0xF0000000;
    const AckNackSubmessage an = {
      {ACKNACK, FLAG_E, 0},
      reader_ent_, writer,
      {nack, 1, bitmap},
      {++acknack_count_}
    };
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
      ACE_DEBUG((LM_ERROR,
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
        if (static_cast<unsigned char>(subm) < n_smkinds) {
          ACE_DEBUG((LM_INFO, "Received submessage type: %C\n",
                     smkinds[static_cast<unsigned char>(subm)]));
        } else {
          ACE_DEBUG((LM_ERROR, "ERROR: Received unknown submessage type: %d\n",
                     int(subm)));
        }
        SubmessageHeader smh;
        if (!(ser >> smh)) {
          ACE_DEBUG((LM_ERROR, "ERROR: in handle_input() failed to deserialize "
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
      ACE_DEBUG((LM_ERROR,
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
      ACE_DEBUG((LM_ERROR,
        "ERROR: recv_nackfrag() failed to deserialize NackFragSubmessage\n"));
      return false;
    }
    if (nf.writerSN.low != 6 && nf.writerSN.low != 7) {
      ACE_DEBUG((LM_ERROR,
                 "ERROR: recv_nackfrag() unexpected NACK_FRAG seq %d\n",
                 nf.writerSN.low));
      return true;
    }
    for (CORBA::ULong i = 0; i < nf.fragmentNumberState.numBits; ++i) {
      if (nf.fragmentNumberState.bitmap[i / 32] & (1 << (31 - (i % 32)))) {
        FragmentNumber_t frag = nf.fragmentNumberState.bitmapBase;
        frag.value += i;
        if (nf.writerSN.low == 6 && frag.value != 2) {
          ACE_DEBUG((LM_ERROR,
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
      ACE_DEBUG((LM_ERROR,
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
      ACE_DEBUG((LM_ERROR,
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
    if (!do_nack_ || data.writerSN.low != 2) { // pretend #2 was lost
      recvd_.insert(data.writerSN.low);
    }
    return true;
  }

  bool recv_hb(Serializer& ser, const ACE_INET_Addr& peer)
  {
    HeartBeatSubmessage hb;
    if (!(ser >> hb)) {
      ACE_DEBUG((LM_ERROR,
        "ERROR: recv_hb() failed to deserialize HeartBeatSubmessage\n"));
      return false;
    }
    ACE_DEBUG((LM_INFO, "recv_hb() first = %d last = %d\n",
               hb.firstSN.low, hb.lastSN.low));
    // pretend #2 was lost
    if (do_nack_ && hb.firstSN.low <= 2 && hb.lastSN.low >= 2) {
      SequenceNumber_t nack = {0, 2};
      ACE_DEBUG((LM_INFO, "recv_hb() requesting retransmit of #2\n"));
      if (!send_an(hb.writerId, nack, peer)) {
        return false;
      }
      do_nack_ = false;
    }
    return true;
  }

  ACE_SOCK_Dgram& sock_;
  CORBA::Long heartbeat_count_, acknack_count_, hbfrag_count_;
  Header hdr_, recv_hdr_;
  ACE_Message_Block recv_mb_;
  bool do_nack_;
  OpenDDS::DCPS::EntityId_t reader_ent_;
  DisjointSequence recvd_;
  static const ACE_CDR::UShort FRAG_SIZE = 1024;
  ACE_CDR::Octet data_for_frag_[FRAG_SIZE];
};


void reactor_wait()
{
  ACE_Time_Value one(1);
  ACE_Reactor::instance()->run_reactor_event_loop(one);
}

void transport_setup()
{
  TransportInst_rch inst =
    TheTransportRegistry->create_inst("my_rtps", "rtps_udp");
  RtpsUdpInst* rtps_inst = dynamic_cast<RtpsUdpInst*>(inst.in());
  //TODO: remove the hard-coded port (below) once the transport knows how to
  //      listen on "any" OS-assigned port.
  rtps_inst->local_address_.set(11694, "localhost");
  rtps_inst->use_multicast_ = false;
  rtps_inst->datalink_release_delay_ = 0;
  rtps_inst->heartbeat_period_ = ACE_Time_Value(0, 500*1000 /*microseconds*/);
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
  part1_locators[0].kind = (part1_addr.get_type() == AF_INET6)
                           ? LOCATOR_KIND_UDPv6 : LOCATOR_KIND_UDPv4;
  part1_locators[0].port = part1_addr.get_port_number();
  RtpsUdpTransport::address_to_bytes(part1_locators[0].address, part1_addr);
  size_t size_locator = 0, padding_locator = 0;
  gen_find_size(part1_locators, size_locator, padding_locator);
  mb_locator.init(size_locator + padding_locator);
  Serializer ser_loc(&mb_locator, ACE_CDR_BYTE_ORDER, Serializer::ALIGN_CDR);
  ser_loc << part1_locators;
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
    addr.set_type(AF_INET6);
    addr.set_address(reinterpret_cast<const char*>(locators[0].address), 16);
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
  OpenDDS::DCPS::GUID_t writer1, reader1, writer2, reader2;
  make_guids(writer1, reader1, writer2, reader2);

  ACE_SOCK_Dgram part1_sock;
  ACE_INET_Addr part1_addr;
  part1_sock.open(part1_addr);
  part1_sock.get_local_addr(part1_addr);
  part1_addr.set(part1_addr.get_port_number(), "localhost");

  SimpleDataWriter sdw2(writer2);
  sdw2.enable_transport(true /*reliable*/);

  SimpleDataReader sdr2(reader2);
  sdr2.enable_transport(true /*reliable*/);


  // "local" setup is now done, start making associations
  ACE_Message_Block mb_locator;
  make_blob(part1_addr, mb_locator);

  AssociationData part1_writer;
  part1_writer.remote_id_ = writer1;
  part1_writer.remote_data_.length(1);
  part1_writer.remote_data_[0].transport_type = "rtps_udp";
  part1_writer.remote_data_[0].data.replace(
    static_cast<CORBA::ULong>(mb_locator.length()), &mb_locator);
  if (!sdr2.associate(part1_writer, false /*active*/)) {
    ACE_DEBUG((LM_DEBUG,
               "SimpleDataReader(reader2) could not associate with writer1\n"));
    return false;
  }
  AssociationData part1_reader = part1_writer;
  part1_reader.remote_id_ = reader1;
  if (!sdw2.associate(part1_reader, true /*active*/)) {
    ACE_DEBUG((LM_DEBUG,
               "SimpleDataWriter(writer1) could not associate with reader1\n"));
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


  // Associations are done, now test the real DR (SimpleDataReader) using our
  // TestParticipant class to interact with it over the socket directly.
  ACE_DEBUG((LM_INFO, ">>> Starting test of DataReader\n"));

  TestParticipant part1(part1_sock, reader1.guidPrefix, reader1.entityId);
  SequenceNumber_t first_seq = {0, 1}, seq = first_seq;
  if (!part1.send_data(writer1.entityId, seq, part2_addr)) {
    return false;
  }
  // this heartbeat isn't final, so reader needs to ack even if it has all data:
  if (!part1.send_hb(writer1.entityId, seq, seq, part2_addr)) {
    return false;
  }
  reactor_wait();

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
    ACE_DEBUG((LM_ERROR, "ERROR: reader2 did not receive expected data\n"));
  }


  // Use the real DDS DataWriter (sdw2) against the test reader
  ACE_DEBUG((LM_INFO, ">>> Starting test of DataWriter\n"));

  SequenceNumber seq_dw2;
  sdw2.send_data(seq_dw2++);  // send #1 - #3, test reader will nack #2
  sdw2.send_data(seq_dw2++);
  sdw2.send_data(seq_dw2++);
  reactor_wait();
  seq_dw2++; // skip #4, transport will generate an inline GAP
  sdw2.send_data(seq_dw2++);  // send #5
  reactor_wait();

  if (part1.recvd_.disjoint() || part1.recvd_.empty()
      || part1.recvd_.high() != seq_dw2.previous()
      || part1.recvd_.low() != SequenceNumber()) {
    ACE_DEBUG((LM_ERROR, "ERROR: reader1 did not receive expected data\n"));
  }

  // cleanup
  sdw2.disassociate(reader1);
  sdr2.disassociate(writer1);
  return true;
}

int ACE_TMAIN(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
  bool ok = false;
  try {
    ok = run_test();
    if (!ok) {
      ACE_DEBUG((LM_ERROR, "ERROR: test failed\n"));
    }
  } catch (const OpenDDS::DCPS::Transport::Exception& e) {
    ACE_DEBUG((LM_ERROR, "EXCEPTION: %C\n", typeid(e).name()));
  } catch (const CORBA::Exception& e) {
    ACE_DEBUG((LM_ERROR, "EXCEPTION: %C\n", e._info().c_str()));
  } catch (const std::exception& e) {
    ACE_DEBUG((LM_ERROR, "EXCEPTION: %C\n", e.what()));
  } catch (...) {
    ACE_DEBUG((LM_ERROR, "Unknown EXCEPTION\n"));
  }
  TheServiceParticipant->shutdown();
  ACE_Thread_Manager::instance()->wait();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
