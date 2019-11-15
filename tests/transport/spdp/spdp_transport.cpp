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

// Declared as spdp_friend. Used to Interact Directly with Spdp.
class DDS_TEST {
public:
  DDS_TEST(const RcHandle<Spdp>& spdp, const GUID_t& guid)
  : spdp_(spdp)
  , guid_(make_guid(guid.guidPrefix, ENTITYID_PARTICIPANT))
  {
  }

  bool check_for_participant(bool expect_participant)
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, spdp_->lock_, true);
    const bool error = spdp_->has_discovered_participant(guid_) != expect_participant;
    if (error) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Error: %C\n"), expect_participant ?
        "Expected to find the participant, but didn't" :
        "Expected not to find the participant, but did"));
    }
    return error;
  }

  void remove_participant()
  {
    ACE_GUARD(ACE_Thread_Mutex, g, spdp_->lock_);
    Spdp::DiscoveredParticipantIter iter = spdp_->participants_.find(guid_);
    if (iter != spdp_->participants_.end()) {
      spdp_->remove_discovered_participant(iter);
    }
  }

  bool get_multicast_address(ACE_INET_Addr& addr) {
    if (!spdp_->tport_) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Null SPDP Transport\n")));
      return false;
    }
    addr = spdp_->tport_->default_multicast_;
    return true;
  }

  const RcHandle<Spdp> spdp_;
  const GUID_t guid_;
};

const bool host_is_bigendian = !ACE_CDR_BYTE_ORDER;

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
    find_size_ulong(size, padding);
    gen_find_size(plist, size, padding);

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
    MessageParser mp(recv_mb_);
    if (!mp.parseHeader()) {
      ACE_ERROR((LM_ERROR,
        "ERROR: in handle_input() failed to deserialize RTPS Header\n"));
      return -1;
    }
    bool ok = true;
    while (ok && mp.remaining()) {
      if (!mp.parseSubmessageHeader()) {
        ok = false;
        break;
      }

      const SubmessageHeader smhdr = mp.submessageHeader();
      if (smhdr.submessageId == DATA) {
        ok = recv_data(mp.serializer(), peer);
      } else {
#ifdef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
        ACE_DEBUG((LM_INFO, "Received submessage type: %u\n", unsigned(smhdr.submessageId)));
#else
        if (static_cast<size_t>(smhdr.submessageId) < gen_OpenDDS_RTPS_SubmessageKind_names_size) {
          ACE_DEBUG((LM_INFO, "Received submessage type: %C\n",
                     gen_OpenDDS_RTPS_SubmessageKind_names[static_cast<size_t>(smhdr.submessageId)]));
        } else {
          ACE_ERROR((LM_ERROR, "ERROR: Received unknown submessage type: %u\n",
                     unsigned(smhdr.submessageId)));
        }
#endif
      }

      if (!ok || !mp.hasNextSubmessage()) {
        break;
      }

      ok = mp.skipToNextSubmessage();
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
  Header hdr_;
  ACE_Message_Block recv_mb_;
};


void reactor_wait()
{
  ACE_Time_Value half_second(0, ACE_ONE_SECOND_IN_USECS / 2);
  ACE_Reactor::instance()->run_reactor_event_loop(half_second);
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
  // Create and initialize RtpsDiscovery
  RtpsDiscovery rd("test");
  const DDS::DomainId_t domain = 0;
  const DDS::DomainParticipantQos qos = TheServiceParticipant->initial_DomainParticipantQos();
  RepoId id = rd.generate_participant_guid();
  const RcHandle<Spdp> spdp(make_rch<Spdp>(domain, ref(id), qos, &rd));

  const DDS::Subscriber_var sVar;
  spdp->init_bit(sVar);

  // Create a "test participant" which will use sockets directly
  // This will act like a remote participant.
  ACE_SOCK_Dgram test_part_sock;
  ACE_INET_Addr test_part_addr;
  if (!open_appropriate_socket_type(test_part_sock, test_part_addr)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: run_test() unable to open test_part_sock\n")));
    return false;
  }
  test_part_sock.get_local_addr(test_part_addr);
  test_part_addr.set(test_part_addr.get_port_number(),
#ifdef OPENDDS_SAFETY_PROFILE
    "127.0.0.1"
#elif defined(ACE_HAS_IPV6)
    "::1/128"
#else
    "localhost"
#endif
    );

  GuidGenerator gen;
  GUID_t test_part_guid;
  gen.populate(test_part_guid);
  test_part_guid.entityId = ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER;
  const GuidPrefix_t& gp = test_part_guid.guidPrefix;
  TestParticipant part1(test_part_sock, gp);

  DDS_TEST spdp_friend(spdp, test_part_guid);

  ACE_INET_Addr send_addr;
  if (!spdp_friend.get_multicast_address(send_addr)) {
    return false;
  }

  // Create a Parameter List
  OpenDDS::RTPS::ParameterList plist;

  const BuiltinEndpointSet_t availableBuiltinEndpoints =
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

  const OpenDDS::RTPS::SPDPdiscoveredParticipantData pdata = {
    {
      DDS::BuiltinTopicKey_t(),
      qos.user_data
    },
    {
      domain,
      "",
      PROTOCOLVERSION,
      {gp[0], gp[1], gp[2], gp[3], gp[4], gp[5],
       gp[6], gp[7], gp[8], gp[9], gp[10], gp[11]},
      VENDORID_OPENDDS,
      false /*expectsIQoS*/,
      availableBuiltinEndpoints,
      0,
      LocatorSeq() /* sedp_multicast */,
      LocatorSeq() /* sedp_unicast */,
      nonEmptyList /*defaultMulticastLocatorList*/,
      nonEmptyList /*defaultUnicastLocatorList*/,
      { 0 /*manualLivelinessCount*/ },
      qos.property,
      {PFLAGS_NO_ASSOCIATED_WRITERS} // opendds_participant_flags
    },
    { // Duration_t (leaseDuration)
      static_cast<CORBA::Long>((rd.resend_period() * 10).value().sec()),
      0 // we are not supporting fractional seconds in the lease duration
    }
  };

  if (OpenDDS::RTPS::ParameterListConverter::to_param_list(pdata, plist) < 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ")
      ACE_TEXT("spdp_transport - run_test - ")
      ACE_TEXT("failed to convert from SPDPdiscoveredParticipantData ")
      ACE_TEXT("to ParameterList\n")));
    return false;
  }

  const SequenceNumber_t first_seq = {0, 1};
  SequenceNumber_t seq;
  bool bfirst = true;
  bool expect_participant = true;

  if (spdp_friend.check_for_participant(false)) {
    return false;
  }

  // Test for performing sequence reset.
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Basic Reset Test\n")));
  for (seq = first_seq; seq.low < 8;) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("seq: %d\n"), seq.low));
    if (!part1.send_data(test_part_guid.entityId, seq, plist, send_addr)) {
      return false;
    }
    reactor_wait();
    if (spdp_friend.check_for_participant(expect_participant)) {
      return false;
    }

    if (bfirst && seq.low == 5) {
      seq.low = 1;
      bfirst = false;
    } else {
      ++seq.low;
    }

    if (!bfirst) {
      if (seq.low == 3) {
        expect_participant = false;
      } else if (seq.low == 4) {
        expect_participant = true;
      }
    }
  }

  // Sequence number starts at 8 and reverts to 6 to verify default reset
  // limits.
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Reset Within Limits Test\n")));
  bfirst = true;
  expect_participant = true;
  while (seq.low < 10) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("seq: %d\n"), seq.low));
    if (!part1.send_data(test_part_guid.entityId, seq, plist, send_addr)) {
      return false;
    }
    reactor_wait();
    if (spdp_friend.check_for_participant(expect_participant)) {
      return false;
    }

    if ((seq.low == 8) && (bfirst)) {
      seq.low = 6;
      bfirst = false;
    } else {
      ++seq.low;
    }
  }

  spdp_friend.remove_participant();

  // Test for possible routing issues when data is arriving via multiple paths.
  // Resends sequence numbers but does not cause a reset.
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Duplicate Sequence Numbers Test\n")));
  for (seq = first_seq; seq.low < 6; seq.low -= 2) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("seq: %d\n"), seq.low));
    if (!part1.send_data(test_part_guid.entityId, seq, plist, send_addr)) {
      return false;
    }
    reactor_wait();
    if (spdp_friend.check_for_participant(expect_participant)) {
      return false;
    }

    for (int i = 0; i < 3; i++) {
      ++seq.low;
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("  seq: %d\n"), seq.low));
      if (!part1.send_data(test_part_guid.entityId, seq, plist, send_addr)) {
        return false;
      }
      reactor_wait();
      if (spdp_friend.check_for_participant(expect_participant)) {
        return false;
      }
    }
  }

  spdp_friend.remove_participant();

  // Test for checking for sequence rollover.  A reset should not occur
  // when the sequence number rolls over to zero from the max value.
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("Overflow Test\n")));
  const SequenceNumber_t overflow_start = { ACE_INT32_MAX, ACE_UINT32_MAX - 5 };
  for (seq = overflow_start; seq.low != 4; seq.low++) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("  seq: %d %u\n"), seq.high, seq.low));
    if (!part1.send_data(test_part_guid.entityId, seq, plist, send_addr)) {
      return false;
    }
    reactor_wait();
    if (spdp_friend.check_for_participant(expect_participant)) {
      return false;
    }

    if (seq.low == ACE_UINT32_MAX) {
      ++seq.high;

      if (seq.high < 0) {
        seq.high = 0;
      }
    }
  }

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
