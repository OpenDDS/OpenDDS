// Test for the mini-transport in SPDP (sequence number handling)

#include "dds/DCPS/transport/rtps_udp/RtpsUdpInst.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "dds/DCPS/transport/framework/NetworkAddress.h"

#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/GuidGenerator.h"
#include "dds/DCPS/RTPS/MessageTypes.h"
#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"
#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/RTPS/ParameterListConverter.h"
#include "dds/DCPS/RTPS/Spdp.h"

#include "dds/DCPS/Service_Participant.h"

#include "ace/Configuration.h"
#include "ace/Reactor.h"
#include "ace/Select_Reactor.h"

#include <ace/OS_main.h>
#include <ace/Thread_Manager.h>
#include <ace/Reactor.h>
#include <ace/SOCK_Dgram.h>

#include <exception>
#include <iostream>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

const bool host_is_bigendian = !ACE_CDR_BYTE_ORDER;
const char* smkinds[] = {"RESERVED_0", "PAD", "RESERVED_2", "RESERVED_3",
  "RESERVED_4", "RESERVED_5", "ACKNACK", "HEARTBEAT", "GAP", "INFO_TS",
  "RESERVED_10", "RESERVED_11", "INFO_SRC", "INFO_REPLY_IP4", "INFO_DST",
  "INFO_REPLY", "RESERVED_16", "RESERVED_17", "NACK_FRAG", "HEARTBEAT_FRAG",
  "RESERVED_20", "DATA", "DATA_FRAG"};
const size_t n_smkinds = sizeof(smkinds) / sizeof(smkinds[0]);

struct TestParticipant: ACE_Event_Handler {
  TestParticipant(ACE_SOCK_Dgram& sock, const OpenDDS::DCPS::GuidPrefix_t& prefix)
    : sock_(sock)
    , recv_mb_(64 * 1024)
  {
    const Header hdr = {
      {'R', 'T', 'P', 'S'}, PROTOCOLVERSION, VENDORID_OPENDDS,
      {prefix[0], prefix[1], prefix[2], prefix[3], prefix[4], prefix[5],
       prefix[6], prefix[7], prefix[8], prefix[9], prefix[10], prefix[11]}
    };
    std::memcpy(&hdr_, &hdr, sizeof(Header));
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
                 const SequenceNumber_t& seq, OpenDDS::RTPS::ParameterList& plist, ACE_INET_Addr& send_to)
  {
    const DataSubmessage ds = {
      {DATA, FLAG_E | FLAG_D, 0},
      0, DATA_OCTETS_TO_IQOS, ENTITYID_UNKNOWN, writer, seq, ParameterList()
    };

    size_t size = 0, padding = 0;
    gen_find_size(hdr_, size, padding);
    gen_find_size(ds, size, padding);

    size += sizeof(plist);
    size +=200;
    ACE_Message_Block mb(size + padding);
    Serializer ser(&mb, host_is_bigendian, Serializer::ALIGN_CDR);

    const ACE_CDR::ULong encap = 0x00000300; // {CDR_LE, options} in BE format

    const bool ok = (ser << hdr_) && (ser << ds) && (ser << encap);
    if (!ok) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize data\n"));
      return false;
    }

    const bool ok2 = (ser << plist);
    if (!ok2) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize data\n"));
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
      ACE_ERROR((LM_ERROR,
        "ERROR: in handle_input() failed to deserialize RTPS Header\n"));
      return -1;
    }
    while (recv_mb_.length() > 3) {
      char subm = recv_mb_.rd_ptr()[0], flags = recv_mb_.rd_ptr()[1];
      ser.swap_bytes((flags & FLAG_E) != ACE_CDR_BYTE_ORDER);
      switch (subm) {
      case DATA:
        if (!recv_data(ser, peer)) return false;
        break;
      default:
        if (static_cast<unsigned char>(subm) < n_smkinds) {
          ACE_DEBUG((LM_INFO, "Received submessage type: %C\n",
                     smkinds[static_cast<unsigned char>(subm)]));
        } else {
          ACE_ERROR((LM_ERROR, "ERROR: Received unknown submessage type: %d\n",
                     int(subm)));
        }
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
    return true;
  }

  ACE_SOCK_Dgram& sock_;
  Header hdr_, recv_hdr_;
  ACE_Message_Block recv_mb_;
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

bool run_test()
{
  // Create a "test participant" which will use sockets directly

  ACE_SOCK_Dgram test_part_sock;
  ACE_INET_Addr test_part_addr;
  if (!open_appropriate_socket_type(test_part_sock, test_part_addr)) {
    std::cerr << "ERROR: run_test() unable to open test_part_sock" << std::endl;
    exit(1);
  }
  test_part_sock.get_local_addr(test_part_addr);
#ifdef OPENDDS_SAFETY_PROFILE
  test_part_addr.set(test_part_addr.get_port_number(), "127.0.0.1");
#else
  test_part_addr.set(test_part_addr.get_port_number(), "localhost");
#endif

  ACE_INET_Addr send_addr("239.255.0.1:7400");

  GuidGenerator gen;
  GUID_t test_part_guid;
  gen.populate(test_part_guid);
  test_part_guid.entityId = ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER;

  TestParticipant part1(test_part_sock, test_part_guid.guidPrefix);

  // Create and initialize RtpsDiscovery

  RtpsDiscovery rd("test");
  const DDS::DomainId_t domain = 0;
  const DDS::DomainParticipantQos qos = TheServiceParticipant->initial_DomainParticipantQos();


  OpenDDS::DCPS::RepoId id = rd.generate_participant_guid();

  const GuidPrefix_t& gp = test_part_guid.guidPrefix;

  const RcHandle<Spdp> spdp(make_rch<Spdp>(domain, ref(id), qos, &rd));



  OpenDDS::RTPS::ParameterList plist;

  BuiltinEndpointSet_t availableBuiltinEndpoints =
    DISC_BUILTIN_ENDPOINT_PARTICIPANT_ANNOUNCER |
    DISC_BUILTIN_ENDPOINT_PARTICIPANT_DETECTOR |
    DISC_BUILTIN_ENDPOINT_PUBLICATION_ANNOUNCER |
    DISC_BUILTIN_ENDPOINT_PUBLICATION_DETECTOR |
    DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_ANNOUNCER |
    DISC_BUILTIN_ENDPOINT_SUBSCRIPTION_DETECTOR |
    BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_WRITER |
    BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER
  ;

  OpenDDS::DCPS::LocatorSeq nonEmptyList(1);
  nonEmptyList.length(1);
  nonEmptyList[0].kind = LOCATOR_KIND_UDPv4;
  nonEmptyList[0].port = 12345;
  std::memset(nonEmptyList[0].address, 0, 12);
  nonEmptyList[0].address[12] = 127;
  nonEmptyList[0].address[13] = 0;
  nonEmptyList[0].address[14] = 0;
  nonEmptyList[0].address[15] = 1;

  OpenDDS::DCPS::LocatorSeq sedp_unicast_, sedp_multicast_;

  const OpenDDS::RTPS::SPDPdiscoveredParticipantData pdata = {
    {
      DDS::BuiltinTopicKey_t(),
      qos.user_data
    },
    {
      PROTOCOLVERSION,
      {gp[0], gp[1], gp[2], gp[3], gp[4], gp[5],
       gp[6], gp[7], gp[8], gp[9], gp[10], gp[11]},
      VENDORID_OPENDDS,
      false /*expectsIQoS*/,
      availableBuiltinEndpoints,
      sedp_multicast_,
      sedp_unicast_,
      nonEmptyList /*defaultMulticastLocatorList*/,
      nonEmptyList /*defaultUnicastLocatorList*/,
      { 0 /*manualLivelinessCount*/ },   //FUTURE: implement manual liveliness
      qos.property,
      {PFLAGS_NO_ASSOCIATED_WRITERS} // opendds_participant_flags
    },
    { // Duration_t (leaseDuration)
       static_cast<CORBA::Long>((rd.resend_period() * 10).sec()),
       0 // we are not supporting fractional seconds in the lease duration
    }
  };

  int irtn = OpenDDS::RTPS::ParameterListConverter::to_param_list(pdata, plist);

  if (irtn < 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("spdp_transport - run_test - ")
      ACE_TEXT("failed to convert from SPDPdiscoveredParticipantData ")
      ACE_TEXT("to ParameterList\n")));
    return false;
  }

  const DDS::Subscriber_var sVar;

  spdp->init_bit(sVar);

  SequenceNumber_t first_seq = {0, 1}, seq = first_seq;
  bool bfirst = true;

  // Test for performing sequence reset.

  for (;;) {
    if (!part1.send_data(test_part_guid.entityId, seq, plist, send_addr)) {
      return false;
    }

    if (seq.low == 5) {
      if (bfirst) {
        seq.low = 1;
        bfirst = false;
      } else {
        ++seq.low;
      }
    } else {
      ++seq.low;
    }

    reactor_wait();

    if (seq.low == 8) {
      break;
    }
  }

  // Sequence number starts at 8 and reverts to 6 to verify default reset
  // limits.

  bfirst = true;

  for (;;) {
    if (!part1.send_data(test_part_guid.entityId, seq, plist, send_addr)) {
      return false;
    }

    if ((seq.low == 8) && (bfirst)) {
      seq.low = 6;
      bfirst = false;
    } else {
      ++seq.low;
    }

    reactor_wait();

    if (seq.low == 10) {
      break;
    }
  }

  // Test for possible routing issues when data is arriving via multiple paths.
  // Resends sequence numbers but does not cause a reset.

  seq = first_seq;

  for (;;) {
    if (!part1.send_data(test_part_guid.entityId, seq, plist, send_addr)) {
      return false;
    }

    reactor_wait();

    for (int i = 0; i < 3; i++)
    {
      ++seq.low;
      if (!part1.send_data(test_part_guid.entityId, seq, plist, send_addr)) {
        return false;
      }
      reactor_wait();
    }

    if (seq.low == 8)
    {
      break;
    }
    seq.low -= 2;
  }

  // Test for checking for sequence rollover.  A reset should not occur
  // when the sequence number rolls over to zero from the max value.

  SequenceNumber_t max_seq = { ACE_INT32_MAX, (ACE_UINT32_MAX - 5) };
  seq = max_seq;

  for (;;) {
    if (!part1.send_data(test_part_guid.entityId, seq, plist, send_addr)) {
      return false;
    }

    if (seq.low == ACE_UINT32_MAX) {
      ++seq.high;

      if (seq.high < 0) {
        seq.high = 0;
      }
    }
    ++seq.low;
    reactor_wait();

    if (seq.low == 2) {
      break;
    }
  }

  reactor_wait();

  return true;
}

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  try {
    ::DDS::DomainParticipantFactory_var dpf =
        TheServiceParticipant->get_domain_participant_factory();
  } catch (const CORBA::BAD_PARAM& ex) {
    ex._tao_print_exception("Exception caught in rtps_reliability.cpp:");
    return 1;
  }

  bool ok = false;
  try {
    ok = run_test();
    if (!ok) {
      ACE_ERROR((LM_ERROR, "ERROR: test failed\n"));
    }
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
