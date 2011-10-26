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
  explicit SimpleDataReader(const RepoId& sub_id) : SimpleTC(sub_id) {}

  void data_received(const ReceivedDataSample& sample)
  {
    ACE_DEBUG((LM_INFO, "data_received with seq#: %q\n",
      sample.header_.sequence_.getValue()));
    recvd_.insert(sample.header_.sequence_);
    if (sample.header_.sequence_ == 5) {
      recvd_.insert(4); // 4 is a deliberate GAP
    }
  }

  void notify_subscription_disconnected(const WriterIdSeq&) {}
  void notify_subscription_reconnected(const WriterIdSeq&) {}
  void notify_subscription_lost(const WriterIdSeq&) {}
  void notify_connection_deleted() {}
  void remove_associations(const WriterIdSeq&, bool) {}

  DisjointSequence recvd_;
};

struct SimpleDataWriter: SimpleTC, TransportSendListener {
  explicit SimpleDataWriter(const RepoId& pub_id) : SimpleTC(pub_id) {}

  void notify_publication_disconnected(const ReaderIdSeq&) {}
  void notify_publication_reconnected(const ReaderIdSeq&) {}
  void notify_publication_lost(const ReaderIdSeq&) {}
  void notify_connection_deleted() {}
  void remove_associations(const ReaderIdSeq&, bool) {}
};

enum RtpsFlags { FLAG_E = 1, FLAG_Q = 2, FLAG_D = 4, FLAG_K = 8 };
const bool host_is_bigendian = !ACE_CDR_BYTE_ORDER;

struct TestParticipant: ACE_Event_Handler {
  TestParticipant(ACE_SOCK_Dgram& sock,
                  const OpenDDS::DCPS::GuidPrefix_t& prefix)
    : sock_(sock), heartbeat_count_(0), recv_mb_(64 * 1024)
  {
    const Header hdr = {
      {'R', 'T', 'P', 'S'}, PROTOCOLVERSION, VENDORID_OPENDDS,
      {prefix[0], prefix[1], prefix[2], prefix[3], prefix[4], prefix[5],
       prefix[6], prefix[7], prefix[8], prefix[9], prefix[10], prefix[11]}
    };
    hdr_ = hdr;
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
      ACE_DEBUG((LM_DEBUG, "ERROR: in send() %p\n", ACE_TEXT("send")));
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
      default:
        ACE_DEBUG((LM_INFO,
          "Received unknown submessage type: %d\n", int(subm)));
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
        "ERROR: in handle_input() failed to deserialize RTPS Header\n"));
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
          ACE_DEBUG((LM_INFO, "recv_an() simulated retransmit %d\n", seq.low));
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

  ACE_SOCK_Dgram& sock_;
  CORBA::Long heartbeat_count_;
  Header hdr_, recv_hdr_;
  ACE_Message_Block recv_mb_;
};

void reactor_wait()
{
  ACE_Time_Value one(1);
  ACE_Reactor::instance()->run_reactor_event_loop(one);
}

void transport_setup()
{
  TransportInst_rch inst = TheTransportRegistry->create_inst("my_rtps",
                                                             "rtps_udp");
  RtpsUdpInst* rtps_inst = dynamic_cast<RtpsUdpInst*>(inst.in());
  //TODO: remove the hard-coded port (below) once the transport knows how to
  //      listen on "any" OS-assigned port.
  rtps_inst->local_address_.set(11694, "localhost");
  rtps_inst->use_multicast_ = false;
  rtps_inst->datalink_release_delay_ = 0;
  rtps_inst->heartbeat_period_ = ACE_Time_Value(0, 100*1000 /*microseconds*/);
  TransportConfig_rch cfg = TheTransportRegistry->create_config("cfg");
  cfg->instances_.push_back(inst);
  TheTransportRegistry->global_config(cfg);
}

bool run_test()
{
  transport_setup();

  // Set up GUIDs for 2 readers and 2 writers.
  // Each pair of reader+writer belongs to the one participant (GuidPrefix_t):
  // Participant 1 contains writer1 and reader1 and will use sockets directly
  // Participant 2 contains writer2 and reader2 and will use the OpenDDS tport
  // Associations: writer1 <-> reader2 and writer2 <-> reader1
  GuidGenerator gen;
  OpenDDS::DCPS::GUID_t writer1;
  gen.populate(writer1);
  {
    OpenDDS::DCPS::EntityId_t entid = {
      {0, 1, 2}, ENTITYKIND_USER_WRITER_WITH_KEY
    };
    writer1.entityId = entid;
  }
  OpenDDS::DCPS::GUID_t reader1 = writer1;
  {
    OpenDDS::DCPS::EntityId_t entid = {
      {0, 1, 3}, ENTITYKIND_USER_READER_WITH_KEY
    };
    reader1.entityId = entid;
  }
  OpenDDS::DCPS::GUID_t writer2;
  gen.populate(writer2);
  {
    OpenDDS::DCPS::EntityId_t entid = {
      {0, 1, 4}, ENTITYKIND_USER_WRITER_WITH_KEY
    };
    writer2.entityId = entid;
  }
  OpenDDS::DCPS::GUID_t reader2 = writer2;
  {
    OpenDDS::DCPS::EntityId_t entid = {
      {0, 1, 5}, ENTITYKIND_USER_READER_WITH_KEY
    };
    reader2.entityId = entid;
  }

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
  LocatorSeq part1_locators;
  part1_locators.length(1);
  part1_locators[0].kind = (part1_addr.get_type() == AF_INET6)
                           ? LOCATOR_KIND_UDPv6 : LOCATOR_KIND_UDPv4;
  part1_locators[0].port = part1_addr.get_port_number();
  RtpsUdpTransport::address_to_bytes(part1_locators[0].address, part1_addr);
  size_t size_locator = 0, padding_locator = 0;
  gen_find_size(part1_locators, size_locator, padding_locator);
  ACE_Message_Block mb_locator(size_locator + padding_locator);
  Serializer ser_loc(&mb_locator, ACE_CDR_BYTE_ORDER, Serializer::ALIGN_CDR);
  ser_loc << part1_locators;
  AssociationData part1_writer;
  part1_writer.remote_id_ = writer1;
  part1_writer.remote_data_.length(1);
  part1_writer.remote_data_[0].transport_type = "rtps_udp";
  part1_writer.remote_data_[0].data.replace(
    static_cast<CORBA::ULong>(mb_locator.length()), &mb_locator);
  if (!sdr2.associate(part1_writer, false)) {
    ACE_DEBUG((LM_DEBUG,
               "SimpleDataReader(reader2) could not associate with writer1\n"));
    return false;
  }
  AssociationData part1_reader = part1_writer;
  part1_reader.remote_id_ = reader1;
  if (!sdw2.associate(part1_reader, true)) {
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
  {
    ACE_Data_Block db(part2_loc[0].data.length(), ACE_Message_Block::MB_DATA,
      reinterpret_cast<const char*>(part2_loc[0].data.get_buffer()),
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
      part2_addr.set_type(AF_INET6);
      part2_addr.set_address(reinterpret_cast<const char*>(locators[0].address),
                             16);
    } else if (locators[0].kind == LOCATOR_KIND_UDPv4) {
      part2_addr.set_type(AF_INET);
      part2_addr.set_address(reinterpret_cast<const char*>(locators[0].address)
                             + 12, 4, 0 /*network order*/);
    } else {
      ACE_DEBUG((LM_DEBUG, "ERROR: unknown locator kind\n"));
      return false;
    }
    part2_addr.set_port_number(locators[0].port);
  }

  // Associations are done, now test the real DR (SimpleDataReader) using our
  // TestParticipant class to interact with it over the socket directly.

  TestParticipant part1(part1_sock, reader1.guidPrefix);
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

  SequenceNumber sn;
  sn.setValue(seq.high, seq.low);
  if (sdr2.recvd_.disjoint() || sdr2.recvd_.empty()
      || sdr2.recvd_.high() != sn) {
    ACE_DEBUG((LM_ERROR, "ERROR: reader2 did not receive expected data\n"));
  }

  //TODO: continue with checking the real DW (SimpleDataWriter) against the test

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
