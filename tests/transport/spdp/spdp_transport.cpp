// Test for the mini-transport in SPDP (sequence number handling)

#include <dds/DCPS/transport/rtps_udp/RtpsUdpInst.h>
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <dds/DCPS/RTPS/GuidGenerator.h>
#include <dds/DCPS/RTPS/MessageTypes.h>
#include <dds/DCPS/RTPS/MessageParser.h>
#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>
#include <dds/DCPS/RTPS/RtpsSubmessageKindTypeSupportImpl.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/RTPS/ParameterListConverter.h>
#include <dds/DCPS/RTPS/Spdp.h>

#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/NetworkResource.h>
#include <dds/DCPS/Service_Participant.h>

#include <dds/OpenDDSConfigWrapper.h>

#include <ace/Configuration.h>
#include <ace/Reactor.h>
#include <ace/Select_Reactor.h>

#include <ace/OS_main.h>
#include <ace/Thread_Manager.h>
#include <ace/Reactor.h>
#include <ace/SOCK_Dgram.h>
#include <ace/OS_NS_arpa_inet.h>
#include <ace/OS_NS_unistd.h>

#include <exception>
#include <iostream>
#include <string>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

// Declared as spdp_friend. Used to Interact Directly with Spdp.
class DDS_TEST {
public:
  DDS_TEST(const RcHandle<Spdp>& spdp, const GUID_t& guid)
  : spdp_(spdp)
  , guid_(make_id(guid.guidPrefix, ENTITYID_PARTICIPANT))
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
      spdp_->purge_discovered_participant(iter);
      spdp_->participants_.erase(iter);
    }
  }

  bool get_multicast_address(ACE_INET_Addr& addr) {
    if (!spdp_->tport_) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Null SPDP Transport\n")));
      return false;
    }
    addr = spdp_->tport_->multicast_address_.to_addr();
    return true;
  }

  const RcHandle<Spdp> spdp_;
  const GUID_t guid_;
};

struct TestParticipant: ACE_Event_Handler {
  TestParticipant(ACE_SOCK_Dgram& sock, const OpenDDS::DCPS::GuidPrefix_t& prefix)
    : sock_(sock)
    , recv_mb_(64 * 1024)
    , user_tag_(0)
  {
    assign(destination_prefix_, GUIDPREFIX_UNKNOWN);
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

  void set_infodst(const GuidPrefix_t& dst)
  {
    assign(destination_prefix_, dst);
  }

  void set_usertag(ACE_CDR::ULong tag)
  {
    user_tag_ = tag;
  }

  bool send_data(const OpenDDS::DCPS::EntityId_t& writer,
                 const SequenceNumber_t& seq, OpenDDS::RTPS::ParameterList& plist, const ACE_INET_Addr& send_to)
  {
    using OpenDDS::DCPS::Encoding;

    const DataSubmessage ds = {
      {DATA, FLAG_E | FLAG_D, 0},
      0, DATA_OCTETS_TO_IQOS, ENTITYID_UNKNOWN, writer, seq, ParameterList()
    };

    const Encoding encoding(Encoding::KIND_XCDR1, OpenDDS::DCPS::ENDIAN_LITTLE);
    const UserTagSubmessage utag = { {SUBMESSAGE_KIND_USER_TAG, FLAG_E, uint32_cdr_size}, user_tag_ };
    InfoDestinationSubmessage idst = { {INFO_DST, FLAG_E, 0}, {0} };
    size_t size = 0;
    serialized_size(encoding, size, hdr_);
    if (user_tag_) {
      serialized_size(encoding, size, utag);
    }
    if (!equal_guid_prefixes(destination_prefix_, GUIDPREFIX_UNKNOWN)) {
      serialized_size(encoding, size, idst);
    }
    serialized_size(encoding, size, ds);
    primitive_serialized_size_ulong(encoding, size);
    serialized_size(encoding, size, plist);

    ACE_Message_Block mb(size);
    Serializer ser(&mb, encoding);

    if (!(ser << hdr_)) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize message headers\n"));
      return false;
    }

    if (user_tag_) {
      if (!(ser << utag)) {
        ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize UserTagSubmessage\n"));
        return false;
      }
      user_tag_ = 0;
    }

    if (!equal_guid_prefixes(destination_prefix_, GUIDPREFIX_UNKNOWN)) {
      assign(idst.guidPrefix, destination_prefix_);
      assign(destination_prefix_, GUIDPREFIX_UNKNOWN);
      if (!(ser << idst)) {
        ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize InfoDestinationSubmessage\n"));
        return false;
      }
    }

    const EncapsulationHeader encap(encoding, MUTABLE);
    if (!(ser << ds && ser << encap)) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize submessage headers\n"));
      return false;
    }

    if (!(ser << plist)) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize payload\n"));
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
        if (gen_OpenDDS_RTPS_SubmessageKind_helper->valid(smhdr.submessageId)) {
          ACE_DEBUG((LM_INFO, "Received submessage type: %C\n",
                     gen_OpenDDS_RTPS_SubmessageKind_helper->get_name(smhdr.submessageId)));
        } else {
          ACE_ERROR((LM_ERROR, "ERROR: Received unknown submessage type: %u\n",
                     unsigned(smhdr.submessageId)));
        }
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
  GuidPrefix_t destination_prefix_;
  ACE_CDR::ULong user_tag_;
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

void to_locator(const ACE_INET_Addr& addr, Locator_t& locator)
{
#ifdef ACE_HAS_IPV6
  if (addr.get_type() == AF_INET6) {
    locator.kind = OpenDDS::RTPS::LOCATOR_KIND_UDPv6;
    locator.port = addr.get_port_number();
    struct sockaddr_in6* in6 = static_cast<struct sockaddr_in6*>(addr.get_addr());
    ACE_OS::memcpy(reinterpret_cast<unsigned char*>(locator.address), &in6->sin6_addr, 16);
  } else
#endif
  {
    locator.kind = OpenDDS::RTPS::LOCATOR_KIND_UDPv4;
    locator.port = addr.get_port_number();
    struct sockaddr_in* sa = static_cast<struct sockaddr_in*>(addr.get_addr());
    std::memset(locator.address, 0, 12);
    ACE_OS::memcpy(reinterpret_cast<unsigned char*>(locator.address) + 12, &sa->sin_addr, 4);
  }
}

bool run_test()
{
  // Create and initialize RtpsDiscovery
  RtpsDiscovery rd("test");
  const ACE_INET_Addr local_addr(u_short(7575), "0.0.0.0");
  rd.config()->spdp_local_address(NetworkAddress(local_addr));
  static const ACE_CDR::ULong user_tag = 0x99887766;
  OpenDDS::RTPS::RtpsDiscoveryConfig::UserTagList ignored;
  ignored.push_back(user_tag);
  rd.config()->ignored_spdp_user_tags(ignored);
  const DDS::DomainId_t domain = 0;
  const DDS::DomainParticipantQos qos = TheServiceParticipant->initial_DomainParticipantQos();
  GUID_t id = rd.generate_participant_guid();
  const RcHandle<Spdp> spdp(make_rch<Spdp>(domain, ref(id), qos, &rd, OpenDDS::XTypes::TypeLookupService_rch()));

  RcHandle<BitSubscriber> bit_subscriber = make_rch<BitSubscriber>();
  spdp->init_bit(bit_subscriber);
  reactor_wait();

  // Check if the port override worked.
  {
    ACE_SOCK_Dgram test_sock;
    if (test_sock.open(local_addr) == 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: run_test() SPDP did not use specified port\n")));
      return false;
    }
  }

  // Create a "test participant" which will use sockets directly
  // This will act like a remote participant.
  ACE_SOCK_Dgram test_part_sock;
  ACE_INET_Addr test_part_addr(u_short(0), "0.0.0.0");
  if (test_part_sock.open(test_part_addr) != 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: run_test() unable to open test_part_sock\n")));
    return false;
  }
  test_part_sock.get_local_addr(test_part_addr);
  test_part_addr.set(test_part_addr.get_port_number(), "127.0.0.1");

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
    BUILTIN_ENDPOINT_PARTICIPANT_MESSAGE_DATA_READER |
    BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_WRITER |
    BUILTIN_ENDPOINT_TYPE_LOOKUP_REQUEST_DATA_READER |
    BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_WRITER |
    BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_READER;

#if OPENDDS_CONFIG_SECURITY
  const DDS::Security::ExtendedBuiltinEndpointSet_t availableExtendedBuiltinEndpoints =
    DDS::Security::TYPE_LOOKUP_SERVICE_REQUEST_SECURE_WRITER |
    DDS::Security::TYPE_LOOKUP_SERVICE_REPLY_SECURE_WRITER |
    DDS::Security::TYPE_LOOKUP_SERVICE_REQUEST_SECURE_READER |
    DDS::Security::TYPE_LOOKUP_SERVICE_REPLY_SECURE_READER;
#endif

  OpenDDS::DCPS::LocatorSeq nonEmptyList(1);
  nonEmptyList.length(1);
  nonEmptyList[0].port = 12345;
  nonEmptyList[0].kind = OpenDDS::RTPS::LOCATOR_KIND_UDPv4;
  std::memset(nonEmptyList[0].address, 0, 12);
  nonEmptyList[0].address[12] = 127;
  nonEmptyList[0].address[13] = 0;
  nonEmptyList[0].address[14] = 0;
  nonEmptyList[0].address[15] = 1;

  size_t addr_count;
  ACE_INET_Addr* addr_array = 0;
  const int ret = ACE::get_ip_interfaces(addr_count, addr_array);
  struct Addr_Deleter {
    Addr_Deleter(ACE_INET_Addr* ptr) : ptr_(ptr) {}
    ~Addr_Deleter() { delete [] ptr_; }
    ACE_INET_Addr* const ptr_;
  } addr_deleter(addr_array);
  if (ret != 0 || addr_count < 1) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: ACE::get_ip_interfaces failed.\n")));
    return false;
  }

  const ACE_CDR::ULong addr_count_ulong = static_cast<ACE_CDR::ULong>(addr_count);
  OpenDDS::DCPS::LocatorSeq unicastLocators(addr_count_ulong);
  unicastLocators.length(addr_count_ulong);
  for (ACE_CDR::ULong i = 0; i < addr_count_ulong; ++i) {
    addr_array[i].set_port_number(12345);
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) spdp_transport.cpp:run_test() addr_array[%d]: %C\n"), i, LogAddr(addr_array[i]).c_str()));
    }
    to_locator(addr_array[i], unicastLocators[i]);
  }

  const OpenDDS::RTPS::SPDPdiscoveredParticipantData pdata = {
    {DDS::BuiltinTopicKey_t(), qos.user_data},
    {
      domain
      , ""
      , PROTOCOLVERSION
      , {gp[0], gp[1], gp[2], gp[3], gp[4], gp[5], gp[6], gp[7], gp[8], gp[9], gp[10], gp[11]}
      , VENDORID_OPENDDS
      , false // expectsIQoS
      , availableBuiltinEndpoints
      , 0
      , unicastLocators // metatrafficUnicastLocatorList
      , nonEmptyList    // metatrafficMulticastLocatorList
      , nonEmptyList    // defaultMulticastLocatorList
      , nonEmptyList    // defaultUnicastLocatorList
      , {0} // manualLivelinessCount
      , qos.property
      , {PFLAGS_THIS_VERSION} // opendds_participant_flags
      , false // opendds_rtps_relay_application_participant
#if OPENDDS_CONFIG_SECURITY
      , availableExtendedBuiltinEndpoints
#endif
      , 0
    },
    { // Duration_t (leaseDuration)
      static_cast<CORBA::Long>((rd.resend_period() * 10).value().sec()),
      0 // we are not supporting fractional seconds in the lease duration
    },
    {0, 0}
  };

  if (!OpenDDS::RTPS::ParameterListConverter::to_param_list(pdata, plist)) {
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

  ACE_DEBUG((LM_DEBUG, "Info Destination Test\n"));
  part1.set_infodst(gp); // The next send_data will insert an InfoDestination submessage before Data.
  // Using the GuidPrefix 'gp' sets the destination GUID to part1's own guid, so it won't be read by 'spdp'.
  if (!part1.send_data(test_part_guid.entityId, seq, plist, send_addr)) {
    ACE_DEBUG((LM_DEBUG, "ERROR: Info Destination test couldn't send\n"));
    return false;
  }
  reactor_wait();
  if (spdp_friend.check_for_participant(false)) {
    ACE_DEBUG((LM_DEBUG, "ERROR: Info Destination test resulted in discovery when it shouldn't\n"));
    return false;
  }

  ACE_DEBUG((LM_DEBUG, "Ignore User Tag Test\n"));
  part1.set_usertag(user_tag); // The next send_data will insert a UserTagSubmessage before Data.
  if (!part1.send_data(test_part_guid.entityId, seq, plist, send_addr)) {
    ACE_DEBUG((LM_DEBUG, "ERROR: Ignore User Tag test couldn't send\n"));
    return false;
  }
  reactor_wait();
  if (spdp_friend.check_for_participant(false)) {
    ACE_DEBUG((LM_DEBUG, "ERROR: Ignore User Tag test resulted in discovery when it shouldn't\n"));
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

  // Sequence number starts at 8 and reverts to 6 to verify default reset limits.
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

  spdp->shutdown();

  return true;
}

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  DDS::DomainParticipantFactory_var dpf;
  bool ok = false;
  try {
    dpf = TheServiceParticipant->get_domain_participant_factory();
    set_DCPS_debug_level(1);
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
