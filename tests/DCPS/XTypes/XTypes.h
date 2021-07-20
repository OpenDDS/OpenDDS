#ifndef XTYPES_H
#define XTYPES_H

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/transport/framework/TransportSendStrategy.h>
#include <dds/DCPS/security/framework/Properties.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/DCPS_Utils.h>

#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

using namespace DDS;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;

bool verbose = false;
bool expect_to_match = true;

int key_value = -1;

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
  if (!topic) {
    ACE_ERROR((LM_ERROR, "ERROR: create_topic failed\n"));
    return false;
  }

  return true;
}

bool wait_for_reader(bool tojoin, DataWriter_var &dw) {
  DDS::WaitSet_var ws = new DDS::WaitSet;
  DDS::StatusCondition_var condition = dw->get_statuscondition();
  condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
  bool success = true;
  DDS::ConditionSeq conditions;
  ReturnCode_t ret = RETCODE_OK;
  ws->attach_condition(condition);
  DDS::Duration_t p = { 5, 0 };
  DDS::PublicationMatchedStatus pms;
  dw->get_publication_matched_status(pms);
  ACE_DEBUG((LM_DEBUG,"Starting wait for reader %C count = %d at %T\n", (tojoin ? "startup":"shutdown"), pms.current_count));
  for (int retries = 3; retries > 0 && (tojoin == (pms.current_count == 0)); --retries) {
    conditions.length(0);
    ret = ws->wait(conditions, p);
    if (ret != RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR: wait on control_dw condition returned %C\n",
                 OpenDDS::DCPS::retcode_to_string(ret)));
      success = false;
      break;
    }
    dw->get_publication_matched_status(pms);
  }
  if (success) {
    ACE_DEBUG((LM_DEBUG, "After wait for reader %C count = %d at %T\n", (tojoin
                                                                         ? "startup"
                                                                         : "shutdown"), pms.current_count));
    if (tojoin != (pms.current_count > 0)) {
      ACE_ERROR((LM_ERROR, "Data reader %C not detected at %T\n", (tojoin
                                                                   ? "startup"
                                                                   : "shutdown")));
    }
  }
  ws->detach_condition(condition);
  return success;
}
bool check_inconsistent_topic_status(Topic_var topic)
{
  DDS::InconsistentTopicStatus status;
  DDS::ReturnCode_t retcode;

  retcode = topic->get_inconsistent_topic_status(status);
  if (retcode != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: get_inconsistent_topic_status failed\n"));
    return false;
  } else if (expect_to_match !=  (status.total_count == 0)) {
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

  dp = dpf->create_participant(0, part_qos, 0, DEFAULT_STATUS_MASK);
}

template<typename T1, typename T2>
ReturnCode_t read_i(const DataReader_var& dr, const T1& pdr, T2& data, bool ignore_no_data = false)
{
  ReadCondition_var dr_rc = dr->create_readcondition(NOT_READ_SAMPLE_STATE,
    ANY_VIEW_STATE,
    ALIVE_INSTANCE_STATE);
  WaitSet_var ws = new WaitSet;
  ws->attach_condition(dr_rc);

  ConditionSeq active;
  const Duration_t max_wait = { 10, 0 };
  ReturnCode_t ret = ws->wait(active, max_wait);

  if (ret == RETCODE_TIMEOUT) {
    ACE_ERROR((LM_ERROR, "ERROR: reader timedout\n"));
  } else if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: Reader: wait returned %C\n",
          OpenDDS::DCPS::retcode_to_string(ret)));
  } else {
    SampleInfoSeq info;

    ret = pdr->take_w_condition(data, info, LENGTH_UNLIMITED, dr_rc);
    if (ignore_no_data && ret == RETCODE_NO_DATA) {
        ret = RETCODE_OK;
    }
    if (ret != RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR: Reader: take_w_condition returned %C\n",
        OpenDDS::DCPS::retcode_to_string(ret)));
    }
  }

  ws->detach_condition(dr_rc);
  dr->delete_readcondition(dr_rc);

  return ret;
}

#endif
