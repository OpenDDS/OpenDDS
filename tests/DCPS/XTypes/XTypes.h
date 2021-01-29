#ifndef XTYPES_H
#define XTYPES_H

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/transport/framework/TransportSendStrategy.h>
#include <dds/DCPS/security/framework/Properties.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

using namespace DDS;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;

bool verbose = false;
bool expect_to_match = true;

int key_value = -1;

bool security_requested = false;

enum AdditionalFieldValue {
  FINAL_STRUCT_AF,
  APPENDABLE_STRUCT_AF,
  MUTABLE_STRUCT_AF,
  NESTED_STRUCT_AF
};

const std::string STRING_26 = "abcdefghijklmnopqrstuvwxyz";
const std::string STRING_20 = "abcdefghijklmnopqrst";

template<typename T>
bool get_topic(T ts, const DomainParticipant_var dp, const std::string& topic_name,
  Topic_var& topic, const std::string& registered_type_name)
{
  if (ts->register_type(dp, registered_type_name.empty() ? "" : registered_type_name.c_str()) != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: register_type failed\n"));
    return false;
  }

  CORBA::String_var type_name;
  if (registered_type_name.empty()) {
    type_name = ts->get_type_name();
  } else {
    type_name = registered_type_name.c_str();
  }

  topic = dp->create_topic(topic_name.c_str(), type_name,
    TOPIC_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
  return true;
}


bool check_inconsistent_topic_status(Topic_var topic)
{
  DDS::InconsistentTopicStatus status;
  DDS::ReturnCode_t retcode;

  retcode = topic->get_inconsistent_topic_status(status);
  if (retcode != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: get_inconsistent_topic_status failed\n"));
    return false;
  } else if (status.total_count != (expect_to_match ? 0 : 1)) {
    ACE_ERROR((LM_ERROR, "ERROR: inconsistent topic count is %d\n", status.total_count));
    return false;
  }
  return true;
}

void append(DDS::PropertySeq& props, const char* name, const char* value)
{
  const DDS::Property_t prop = { name, value, false /*propagate*/ };
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

void create_participant(const DomainParticipantFactory_var& dpf, DomainParticipant_var& dp) {
  DDS::DomainParticipantQos part_qos;
  dpf->get_default_participant_qos(part_qos);

  if (security_requested) {
#if defined(OPENDDS_SECURITY)
    if (TheServiceParticipant->get_security()) {
      using namespace DDS::Security::Properties;
      DDS::PropertySeq& props = part_qos.property.value;

      append(props, AuthIdentityCA, "file:../../security/certs/identity/identity_ca_cert.pem");
      append(props, AuthIdentityCertificate, "file:../../security/certs/identity/test_participant_02_cert.pem");
      append(props, AuthPrivateKey, "file:../../security/certs/identity/test_participant_02_private_key.pem");
      append(props, AccessPermissionsCA, "file:../../security/certs/permissions/permissions_ca_cert.pem");
      append(props, AccessGovernance, "file:../../security/attributes/governance/governance_AU_UA_ND_NL_NR_signed.p7s");
      append(props, AccessPermissions, "file:../../security/attributes/permissions/permissions_test_participant_02_allowall_signed.p7s");
    }
#endif
  }

  dp = dpf->create_participant(0, PARTICIPANT_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
}
#endif
