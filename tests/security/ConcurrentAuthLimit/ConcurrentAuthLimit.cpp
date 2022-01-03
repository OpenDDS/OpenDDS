#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/RTPS/BaseMessageTypes.h>
#include <dds/DCPS/RTPS/GuidGenerator.h>
#include <dds/DCPS/RTPS/MessageTypes.h>
#include <dds/DCPS/RTPS/ParameterListConverter.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/security/framework/Properties.h>
#include <dds/DCPS/security/AuthenticationBuiltInImpl.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdpInst.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/security/BuiltInPlugins.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/SOCK_Dgram.h>
#include <ace/SOCK_Dgram_Mcast.h>

#include <stdexcept>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

void append(DDS::PropertySeq& props, const char* name, const OpenDDS::DCPS::String& value, bool propagate = false)
{
  const DDS::Property_t prop = {name, value.c_str(), propagate};
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

bool receive(const OpenDDS::DCPS::GUID_t& participant_guid,
             ACE_SOCK_Dgram_Mcast& multicast_socket)
{
  // Wait for an SPDP announcement before proceeding.
  ACE_Message_Block buff(64 * 1024);

  ACE_INET_Addr remote;
  buff.reset();

  const ssize_t bytes = multicast_socket.recv(buff.wr_ptr(), buff.space(), remote);
  buff.wr_ptr(bytes);

  const Encoding encoding_plain_native(Encoding::KIND_XCDR1);
  OpenDDS::DCPS::Serializer ser(&buff, encoding_plain_native);
  Header header;
  if (!(ser >> header)) {
    return EXIT_FAILURE;
  }

  return make_part_guid(header.guidPrefix) == participant_guid;
}

OpenDDS::Security::SPDPdiscoveredParticipantData
participant_data(DDS::DomainId_t domain,
                 const GuidPrefix_t& gp,
                 const DDS::DomainParticipantQos& qos,
                 const ACE_INET_Addr& addr)
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
    BUILTIN_ENDPOINT_TYPE_LOOKUP_REPLY_DATA_READER |
    DDS::Security::SEDP_BUILTIN_PUBLICATIONS_SECURE_WRITER |
    DDS::Security::SEDP_BUILTIN_PUBLICATIONS_SECURE_READER |
    DDS::Security::SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_WRITER |
    DDS::Security::SEDP_BUILTIN_SUBSCRIPTIONS_SECURE_READER |
    DDS::Security::BUILTIN_PARTICIPANT_MESSAGE_SECURE_WRITER |
    DDS::Security::BUILTIN_PARTICIPANT_MESSAGE_SECURE_READER |
    DDS::Security::BUILTIN_PARTICIPANT_STATELESS_MESSAGE_WRITER |
    DDS::Security::BUILTIN_PARTICIPANT_STATELESS_MESSAGE_READER |
    DDS::Security::BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_WRITER |
    DDS::Security::BUILTIN_PARTICIPANT_VOLATILE_MESSAGE_SECURE_READER |
    DDS::Security::SPDP_BUILTIN_PARTICIPANT_SECURE_WRITER |
    DDS::Security::SPDP_BUILTIN_PARTICIPANT_SECURE_READER;

  const DDS::Security::ExtendedBuiltinEndpointSet_t availableExtendedBuiltinEndpoints =
    DDS::Security::TYPE_LOOKUP_SERVICE_REQUEST_WRITER_SECURE |
    DDS::Security::TYPE_LOOKUP_SERVICE_REPLY_WRITER_SECURE |
    DDS::Security::TYPE_LOOKUP_SERVICE_REQUEST_READER_SECURE |
    DDS::Security::TYPE_LOOKUP_SERVICE_REPLY_READER_SECURE;

  ACE_INET_Addr bogus(12345, "127.0.0.1");
  OpenDDS::DCPS::LocatorSeq nonEmptyList(1);
  nonEmptyList.length(1);
  address_to_locator(nonEmptyList[0], bogus);

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

  const ACE_CDR::ULong addr_count_ulong = static_cast<ACE_CDR::ULong>(addr_count);
  OpenDDS::DCPS::LocatorSeq unicastLocators(addr_count_ulong);
  unicastLocators.length(addr_count_ulong);
  for (ACE_CDR::ULong i = 0; i < addr_count_ulong; ++i) {
    // Set a port number to avoid send errors.
    addr_array[i].set_port_number(addr.get_port_number());
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) addr_array[%d]: %C\n"), i, LogAddr(addr_array[i]).c_str()));
    }
    address_to_locator(unicastLocators[i], addr_array[i]);
  }


  DDS::Security::PermissionsToken identity_token;
  identity_token.class_id = OpenDDS::Security::Identity_Status_Token_Class_Id;

  DDS::Security::Token permissions_token;
  DDS::Security::PropertyQosPolicy property;
  DDS::Security::ParticipantSecurityInfo  security_info =
    { DDS::Security::PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_VALID,
      DDS::Security::PLUGIN_PARTICIPANT_SECURITY_ATTRIBUTES_FLAG_IS_VALID };
  DDS::Security::IdentityToken identity_status_token;

  const OpenDDS::Security::SPDPdiscoveredParticipantData pdata =
    {
      OpenDDS::Security::DPDK_ENHANCED, // dataKind
      { // ddsParticipantDataSecure
        {
          {
            DDS::BuiltinTopicKey_t()
            , qos.user_data
          }
          , identity_token
          , permissions_token
          , property
          , security_info
          , availableExtendedBuiltinEndpoints // extended_builtin_endpoints
        }
        , identity_status_token
      },
      { // participantProxy
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
        , availableExtendedBuiltinEndpoints
      },
      {300, 0}, // leaseDuration
      {0, 0}, // discoveredAt
    };
  return pdata;
}

void send(ACE_SOCK_Dgram socket,
          const OpenDDS::DCPS::GUID_t& from,
          OpenDDS::DCPS::SequenceNumber& seq,
          const ACE_INET_Addr& to,
          const OpenDDS::Security::SPDPdiscoveredParticipantData& pdata)
{
  const GuidPrefix_t& from_prefix = from.guidPrefix;

  const Header hdr = {{'R', 'T', 'P', 'S'}, PROTOCOLVERSION, VENDORID_OPENDDS,
                      {from_prefix[0], from_prefix[1], from_prefix[2], from_prefix[3], from_prefix[4], from_prefix[5], from_prefix[6], from_prefix[7], from_prefix[8], from_prefix[9], from_prefix[10], from_prefix[11]}};

  OpenDDS::RTPS::ParameterList plist;
  OpenDDS::RTPS::ParameterListConverter::to_param_list(pdata, plist);

  using OpenDDS::DCPS::Encoding;
  const DataSubmessage ds = {
    {DATA, FLAG_E | FLAG_D, 0}, 0, DATA_OCTETS_TO_IQOS, ENTITYID_UNKNOWN, from.entityId, to_rtps_seqnum(seq), ParameterList()
  };
  ++seq;
  const Encoding encoding(Encoding::KIND_XCDR1, OpenDDS::DCPS::ENDIAN_LITTLE);
  size_t size = 0;
  serialized_size(encoding, size, hdr);
  serialized_size(encoding, size, ds);
  primitive_serialized_size_ulong(encoding, size);
  serialized_size(encoding, size, plist);
  ACE_Message_Block mb(size);
  Serializer ser(&mb, encoding);
  const EncapsulationHeader encap (encoding, MUTABLE);

  ser << hdr;
  ser << ds;
  ser << encap;
  ser << plist;

  socket.send(mb.rd_ptr(), mb.length(), to);
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  DDS::DomainParticipantFactory_var dpf = TheServiceParticipant->get_domain_participant_factory(argc, argv);

  // Parse options.
  OpenDDS::DCPS::String DDS_ROOT;
  bool no_limit = false;

  const char* value = ACE_OS::getenv("DDS_ROOT");
  if (value) {
    DDS_ROOT = value;
  }

  value = ACE_OS::getenv("no_limit");
  if (value && strcmp(value, "true") == 0) {
    no_limit = true;
  }

  ACE_DEBUG((LM_DEBUG, "DDS_ROOT = %C\n", DDS_ROOT.c_str()));
  ACE_DEBUG((LM_DEBUG, "no_limit = %d\n", no_limit));

  const DDS::DomainId_t domain = 4;
  const RcHandle<RtpsDiscovery> disc = make_rch<RtpsDiscovery>("RtpsDiscovery");

  if (!no_limit) {
    disc->config()->max_participants_in_authentication(1);
  }

  // Create sockets for the SPDP writers.
  GuidGenerator guid_generator;

  GUID_t writer1_guid;
  guid_generator.populate(writer1_guid);
  writer1_guid.entityId = ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER;
  ACE_DEBUG((LM_DEBUG, "Writer1 guid is %C\n", LogGuid(writer1_guid).c_str()));
  SequenceNumber writer1_sequence;

  const ACE_INET_Addr writer1_addr(u_short(7575), "0.0.0.0");
  ACE_SOCK_Dgram writer1_socket;
  if (writer1_socket.open(writer1_addr)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: failed to open writer1 socket\n")));
    return EXIT_FAILURE;
  }

  GUID_t writer2_guid;
  guid_generator.populate(writer2_guid);
  writer2_guid.entityId = ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER;
  ACE_DEBUG((LM_DEBUG, "Writer2 guid is %C\n", LogGuid(writer2_guid).c_str()));
  SequenceNumber writer2_sequence;

  const ACE_INET_Addr writer2_addr(u_short(7576), "0.0.0.0");
  ACE_SOCK_Dgram writer2_socket;
  if (writer2_socket.open(writer2_addr)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: failed to open writer2 socket\n")));
    return EXIT_FAILURE;
  }

  const u_short port_common = disc->config()->port_common(domain);
  const ACE_INET_Addr multicast_address = disc->config()->multicast_address(port_common);
  ACE_DEBUG((LM_DEBUG, "multicast_address = %C\n", LogAddr(multicast_address).c_str()));
  ACE_SOCK_Dgram_Mcast multicast_socket;
#ifdef ACE_HAS_MAC_OSX
  multicast_socket.opts(ACE_SOCK_Dgram_Mcast::OPT_BINDADDR_NO |
                        ACE_SOCK_Dgram_Mcast::DEFOPT_NULLIFACE);
#endif
  if (multicast_socket.join(multicast_address, 1, 0)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: failed to join multicast group\n")));
    return EXIT_FAILURE;
  }

  // Create a participant.
  TheServiceParticipant->set_security(true);
  TheServiceParticipant->add_discovery(disc);
  TheServiceParticipant->set_default_discovery(disc->key());

  DDS::DomainParticipantQos participant_qos;
  dpf->get_default_participant_qos(participant_qos);

  DDS::PropertySeq& props = participant_qos.property.value;

  const OpenDDS::DCPS::String prefix = "file:" + DDS_ROOT + "/tests/security/";
  append(props, DDS::Security::Properties::AuthIdentityCA, prefix + "certs/identity/identity_ca_cert.pem");
  append(props, DDS::Security::Properties::AuthPrivateKey, prefix + "certs/identity/test_participant_01_private_key.pem");
  append(props, DDS::Security::Properties::AuthIdentityCertificate, prefix + "certs/identity/test_participant_01_cert.pem");
  append(props, DDS::Security::Properties::AccessPermissionsCA, prefix + "certs/permissions/permissions_ca_cert.pem");
  append(props, DDS::Security::Properties::AccessGovernance, "file:./governance_signed.p7s");
  append(props, DDS::Security::Properties::AccessPermissions, "file:./permissions_1_signed.p7s");

  DDS::DomainParticipant_var participant = dpf->create_participant(domain, participant_qos, 0, 0);
  OpenDDS::DCPS::DomainParticipantImpl* participant_impl =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant.in());

  const OpenDDS::DCPS::GUID_t participant_guid = participant_impl->get_id();
  ACE_DEBUG((LM_DEBUG, "Participant guid is %C\n", LogGuid(participant_guid).c_str()));

  while (!receive(participant_guid, multicast_socket)) {}

  // Send an SPDP message from writer1.
  send(writer1_socket, writer1_guid, writer1_sequence, multicast_address, participant_data(domain, writer1_guid.guidPrefix, DDS::DomainParticipantQos(), writer1_addr));

  // Send an SPDP message from writer2.
  send(writer2_socket, writer2_guid, writer2_sequence, multicast_address, participant_data(domain, writer2_guid.guidPrefix, DDS::DomainParticipantQos(), writer2_addr));

  // Sleep to let SPDP process.
  ACE_OS::sleep(1);

  int status = EXIT_SUCCESS;

  if (!disc->has_domain_participant(domain, participant_guid, make_part_guid(writer1_guid))) {
    ACE_ERROR((LM_ERROR, "Participant has not discovered writer1\n"));
    status = EXIT_FAILURE;
  }

  if (no_limit) {
    if (!disc->has_domain_participant(domain, participant_guid, make_part_guid(writer2_guid))) {
      ACE_ERROR((LM_ERROR, "Participant has not discovered writer2 when it should have\n"));
      status = EXIT_FAILURE;
    }
  } else {
    if (disc->has_domain_participant(domain, participant_guid, make_part_guid(writer2_guid))) {
      ACE_ERROR((LM_ERROR, "Participant has discovered writer2 when it should not have\n"));
      status = EXIT_FAILURE;
    }
  }

  participant->delete_contained_entities();
  dpf->delete_participant(participant);
  TheServiceParticipant->shutdown();

  return status;
}
