#include <dds/DCPS/RTPS/BaseMessageTypes.h>
#include <dds/DCPS/RTPS/GuidGenerator.h>
#include <dds/DCPS/RTPS/MessageTypes.h>
#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/RTPS/ParameterListConverter.h>
#include <dds/DCPS/RTPS/Spdp.h>

#include <dds/DCPS/security/framework/Properties.h>

#include <dds/DCPS/transport/framework/NetworkAddress.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdpInst.h>
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/RepoIdBuilder.h>
#include <dds/DCPS/Service_Participant.h>

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

const char auth_ca_file_from_tests[] = "certs/identity/identity_ca_cert.pem";
const char perm_ca_file_from_tests[] = "certs/permissions/permissions_ca_cert.pem";
const char id_cert_file_from_tests[] = "certs/identity/test_participant_01_cert.pem";
const char id_key_file_from_tests[] = "certs/identity/test_participant_01_private_key.pem";
const char governance_file[] = "file:./governance_signed.p7s";
const char permissions_file[] = "file:./permissions_1_signed.p7s";

void append(DDS::PropertySeq& props, const char* name, const char* value, bool propagate = false)
{
  const DDS::Property_t prop = {name, value, propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

void add_security_properties(DDS::PropertySeq& props)
{
  // Determine the path to the keys
  const OpenDDS::DCPS::String path_to_tests = "file:../";
  const OPENDDS_STRING auth_ca_file = path_to_tests + auth_ca_file_from_tests;
  const OPENDDS_STRING perm_ca_file = path_to_tests + perm_ca_file_from_tests;
  const OPENDDS_STRING id_cert_file = path_to_tests + id_cert_file_from_tests;
  const OPENDDS_STRING id_key_file = path_to_tests + id_key_file_from_tests;
  if (TheServiceParticipant->get_security()) {
    append(props, DDS::Security::Properties::AuthIdentityCA, auth_ca_file.c_str());
    append(props, DDS::Security::Properties::AuthIdentityCertificate, id_cert_file.c_str());
    append(props, DDS::Security::Properties::AuthPrivateKey, id_key_file.c_str());
    append(props, DDS::Security::Properties::AccessPermissionsCA, perm_ca_file.c_str());
    append(props, DDS::Security::Properties::AccessGovernance, governance_file);
    append(props, DDS::Security::Properties::AccessPermissions, permissions_file);
  }
}

// Declared as spdp_friend. Used to Interact Directly with Spdp.
class DDS_TEST {
public:
  static const GUID_t writerId[2];
  static const GUID_t readerId;

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
    const size_t size = spdp_->participants_.size();
    ACE_DEBUG((LM_INFO, ACE_TEXT("remove_participant: size=%d\n"), size));
    for (Spdp::DiscoveredParticipantIter i = spdp_->participants_.begin(); i != spdp_->participants_.end(); i = spdp_->participants_.begin()) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("remove_participant: %C %C\n"), OPENDDS_STRING(GuidConverter(i->first)).c_str(), LogAddr(i->second.local_address_).c_str()));
      spdp_->remove_discovered_participant(i);
    }
  }

  bool get_multicast_address(ACE_INET_Addr& addr)
  {
    if (!spdp_->tport_) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Null SPDP Transport\n")));
      return false;
    }
    addr = spdp_->tport_->multicast_address_;
    return true;
  }

private:
  const RcHandle<Spdp> spdp_;
  const GUID_t guid_;
  static GUID_t createID(long participantId, long key, CORBA::Octet kind)
  {
    OpenDDS::DCPS::RepoIdBuilder idbd;
    idbd.federationId(0x01234567); // guidPrefix1
    idbd.participantId(participantId); // guidPrefix2
    idbd.entityKey(key);
    idbd.entityKind(kind);
    return idbd;
  }
};

const GUID_t DDS_TEST::writerId[2] = {
  createID(0x11111111, 0x111111, OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY),
  createID(0x22222222, 0x222222, OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY)
};
const GUID_t DDS_TEST::readerId = createID(0x33333333, 0x333333, OpenDDS::DCPS::ENTITYKIND_USER_READER_WITH_KEY);

struct TestParticipant : ACE_Event_Handler {
  TestParticipant(ACE_SOCK_Dgram& sock, const OpenDDS::DCPS::GuidPrefix_t& px)
    : sock_(sock)
    , recv_mb_(64 * 1024)
  {
    const Header hdr = {{'R', 'T', 'P', 'S'}, PROTOCOLVERSION, VENDORID_OPENDDS,
      {px[0], px[1], px[2], px[3], px[4], px[5], px[6], px[7], px[8], px[9], px[10], px[11]}
    };
    std::memcpy(&hdr_, &hdr, sizeof(Header));
    if (ACE_Reactor::instance()->register_handler(sock_.get_handle(), this, READ_MASK) == -1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: TestParticipant register_handler failed\n")));
      throw std::exception();
    }
  }

  ~TestParticipant()
  {
    if (ACE_Reactor::instance()->remove_handler(sock_.get_handle(), ALL_EVENTS_MASK | DONT_CALL) == -1) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: ~TestParticipant remove_handler failed\n")));
    }
  }

  bool send_data(const OpenDDS::DCPS::EntityId_t& writer, const SequenceNumber_t& seq, OpenDDS::RTPS::ParameterList& plist, const ACE_INET_Addr& send_to)
  {
    using OpenDDS::DCPS::Encoding;
    const DataSubmessage ds = {
      {DATA, FLAG_E | FLAG_D, 0}, 0, DATA_OCTETS_TO_IQOS, ENTITYID_UNKNOWN, writer, seq, ParameterList()
    };
    const Encoding encoding(Encoding::KIND_XCDR1, OpenDDS::DCPS::ENDIAN_LITTLE);
    size_t size = 0;
    serialized_size(encoding, size, hdr_);
    serialized_size(encoding, size, ds);
    primitive_serialized_size_ulong(encoding, size);
    serialized_size(encoding, size, plist);
    ACE_Message_Block mb(size);
    Serializer ser(&mb, encoding);
    const EncapsulationHeader encap (encoding, MUTABLE);
    if (!(ser << hdr_ && ser << ds && ser << encap)) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize headers\n"));
      return false;
    }
    if (!(ser << plist)) {
      ACE_DEBUG((LM_DEBUG, "ERROR: failed to serialize payload\n"));
      return false;
    }
    return send(mb, send_to);
  }

//int handle_input(ACE_HANDLE)
  int handle_input(ACE_HANDLE fd = ACE_INVALID_HANDLE)
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
      ACE_ERROR((LM_ERROR, "ERROR: in handle_input() failed to deserialize RTPS Header\n"));
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
          ACE_DEBUG((LM_INFO, "Received submessage type: %C\n", gen_OpenDDS_RTPS_SubmessageKind_names[static_cast<size_t>(smhdr.submessageId)]));
        } else {
          ACE_ERROR((LM_ERROR, "ERROR: Received unknown submessage type: %u\n", unsigned(smhdr.submessageId)));
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

private:
  bool send(const ACE_Message_Block& mb, const ACE_INET_Addr& send_to)
  {
    if (sock_.send(mb.rd_ptr(), mb.length(), send_to) < 0) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("ERROR: TestParticipant::send() failed\n")));
      return false;
    }
    return true;
  }

  bool recv_data(Serializer& ser, const ACE_INET_Addr&)
  {
    DataSubmessage data;
    if (!(ser >> data)) {
      ACE_ERROR((LM_ERROR, "ERROR: recv_data() failed to deserialize DataSubmessage\n"));
      return false;
    }
    ACE_DEBUG((LM_INFO, "recv_data() seq = %d\n", data.writerSN.low));
    if (data.smHeader.submessageLength) {
      ser.skip(data.smHeader.submessageLength - 20); // 20 == size of Data headers after smHeader (assuming no Inline QoS)
    } else {
      ser.skip(8); // our data payloads are 8 bytes
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

void to_locator(const ACE_INET_Addr& addr, Locator_t& locator)
{
#ifdef ACE_HAS_IPV6
  if (addr.get_type() == AF_INET6) {
    locator.kind = LOCATOR_KIND_UDPv6;
    struct sockaddr_in6* in6 = static_cast<struct sockaddr_in6*>(addr.get_addr());
    ACE_OS::memcpy(reinterpret_cast<unsigned char*>(locator.address), &in6->sin6_addr, 16);
  } else
#endif
  {
    locator.kind = LOCATOR_KIND_UDPv4;
    struct sockaddr_in* sa = static_cast<struct sockaddr_in*>(addr.get_addr());
    std::memset(locator.address, 0, 12);
    ACE_OS::memcpy(reinterpret_cast<unsigned char*>(locator.address) + 12, &sa->sin_addr, 4);
  }
}

OpenDDS::RTPS::SPDPdiscoveredParticipantData discovered_participant_data(
  const DDS::DomainId_t& domain, const GuidPrefix_t& gp, const DDS::DomainParticipantQos& qos, const RtpsDiscovery& rd)
{
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

#ifdef OPENDDS_SECURITY
  const DDS::Security::ExtendedBuiltinEndpointSet_t availableExtendedBuiltinEndpoints =
    DDS::Security::TYPE_LOOKUP_SERVICE_REQUEST_WRITER_SECURE |
    DDS::Security::TYPE_LOOKUP_SERVICE_REPLY_WRITER_SECURE |
    DDS::Security::TYPE_LOOKUP_SERVICE_REQUEST_READER_SECURE |
    DDS::Security::TYPE_LOOKUP_SERVICE_REPLY_READER_SECURE;
#endif

  OpenDDS::DCPS::LocatorSeq nonEmptyList(1);
  nonEmptyList.length(1);
  nonEmptyList[0].port = 12345;
  nonEmptyList[0].kind = LOCATOR_KIND_UDPv4;
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
    throw std::runtime_error("ACE::get_ip_interfaces failed");
  }

  OpenDDS::DCPS::LocatorSeq unicastLocators(addr_count);
  unicastLocators.length(addr_count);
  for (size_t i = 0; i < addr_count; ++i) {
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) addr_array[%d]: %C\n"), i, LogAddr(addr_array[i]).c_str()));
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
#ifdef OPENDDS_SECURITY
      , availableExtendedBuiltinEndpoints
#endif
    },
    {static_cast<CORBA::Long>((rd.resend_period() * 10).value().sec()), 0}, // leaseDuration
    {0, 0}, 0
  };
  return pdata;
}

bool run_test()
{
  // Create and initialize RtpsDiscovery
  RtpsDiscovery rd("test");
  rd.config()->use_ncm(false);
  const ACE_INET_Addr local_addr(u_short(7575), "0.0.0.0");
  rd.config()->spdp_local_address(local_addr);
  const DDS::DomainId_t domain = 4;
  DDS::DomainParticipantQos qos = TheServiceParticipant->initial_DomainParticipantQos();
  add_security_properties(qos.property.value);

  GUID_t id = DDS_TEST::readerId;
  const RcHandle<Spdp> spdp(make_rch<Spdp>(domain, ref(id), qos, &rd, OpenDDS::XTypes::TypeLookupService_rch()));
  ACE_DEBUG((LM_INFO, ACE_TEXT("id: %C %C\n"), OPENDDS_STRING(GuidConverter(id)).c_str(), LogAddr(rd.config()->spdp_local_address()).c_str()));

  const DDS::Subscriber_var sVar;
  spdp->init_bit(sVar);
  reactor_wait();

  // Check if the port override worked.
  {
    ACE_SOCK_Dgram test_sock;
    if (test_sock.open(local_addr) == 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: run_test() SPDP did not use specified port\n")));
      return false;
    }
  }

  // Create a "remote" participant to use sockets directly
  ACE_SOCK_Dgram test_part_sock;
  ACE_INET_Addr test_part_addr(u_short(0), "0.0.0.0");
  if (test_part_sock.open(test_part_addr) != 0) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: run_test() unable to open test_part_sock\n")));
    return false;
  }
  test_part_sock.get_local_addr(test_part_addr);
  test_part_addr.set(test_part_addr.get_port_number(), "127.0.0.1");
//[
  GuidGenerator gen;
  GUID_t test_part_guid;
  gen.populate(test_part_guid);
  test_part_guid.entityId = ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER;
//]
  //GUID_t test_part_guid = DDS_TEST::writerId[0]; //??
  const GuidPrefix_t& gp = test_part_guid.guidPrefix;
  TestParticipant part1(test_part_sock, gp);

  DDS_TEST spdp_friend(spdp, test_part_guid);

  ACE_INET_Addr send_to;
  if (!spdp_friend.get_multicast_address(send_to)) {
    return false;
  }

  const OpenDDS::RTPS::SPDPdiscoveredParticipantData pdata = discovered_participant_data(domain, gp, qos, rd);
  OpenDDS::RTPS::ParameterList plist;
  if (!OpenDDS::RTPS::ParameterListConverter::to_param_list(pdata, plist)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to convert SPDPdiscoveredParticipantData to ParameterList\n")));
    return false;
  }

  if (spdp_friend.check_for_participant(false)) {
    return false;
  }

  for (SequenceNumber_t seq = {0, 1}; seq.low < 4; ++seq.low) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("seq:%d send_to:%C\n"), seq.low, LogAddr(send_to).c_str()));
    if (!part1.send_data(test_part_guid.entityId, seq, plist, send_to)) {
      return false;
    }
    reactor_wait();
    if (spdp_friend.check_for_participant(true)) {
      return false;
    }
  }

  spdp_friend.remove_participant();
  return true;
}

int ACE_TMAIN(int, ACE_TCHAR*[])
{
  bool ok = false;
  try {
    DDS::DomainParticipantFactory_var dpf = TheServiceParticipant->get_domain_participant_factory();
    set_DCPS_debug_level(1);
    ok = run_test();
    if (!ok) {
      ACE_ERROR((LM_ERROR, "ERROR: test failed\n"));
    }
  } catch (const CORBA::BAD_PARAM& e) {
    e._tao_print_exception("ERROR: CORBA::BAD_PARAM:");
  } catch (const CORBA::Exception& e) {
    ACE_ERROR((LM_ERROR, "ERROR: CORBA::Exception: %C\n", e._info().c_str()));
  } catch (const std::exception& e) {
    ACE_ERROR((LM_ERROR, "ERROR: std::exception: %C\n", e.what()));
  } catch (...) {
    ACE_ERROR((LM_ERROR, "ERROR: EXCEPTION ...\n"));
  }
  TheServiceParticipant->shutdown();
  ACE_Thread_Manager::instance()->wait();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
