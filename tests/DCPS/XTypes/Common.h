#ifndef COMMON_H
#define COMMON_H

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
#  include <dds/DCPS/transport/tcp/Tcp.h>
#  ifdef OPENDDS_SECURITY
#    include <dds/DCPS/security/BuiltInPlugins.h>
#  endif
#endif

using namespace DDS;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;

bool verbose = false;
bool expect_inconsistent_topic = false;
bool expect_incompatible_qos = false;

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
  if (ts->register_type(dp, registered_type_name.empty() ? "" : registered_type_name.c_str()) != RETCODE_OK) {
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
  WaitSet_var ws = new DDS::WaitSet;
  StatusCondition_var condition = dw->get_statuscondition();
  condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS);
  ws->attach_condition(condition);

  bool success = true;
  Duration_t timeout = { 5, 0 };
  PublicationMatchedStatus pms;
  dw->get_publication_matched_status(pms);

  ACE_DEBUG((LM_DEBUG,"Starting wait for reader %C -- Count = %d at %T\n",
             (tojoin ? "startup" : "shutdown"), pms.current_count));
  for (int retries = 3; retries > 0 && (tojoin == (pms.current_count == 0)); --retries) {
    ConditionSeq conditions;
    const ReturnCode_t ret = ws->wait(conditions, timeout);
    if (ret != RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR: wait on control_dw condition returned %C\n",
                 OpenDDS::DCPS::retcode_to_string(ret)));
      success = false;
      break;
    }
    dw->get_publication_matched_status(pms);
  }

  if (success) {
    ACE_DEBUG((LM_DEBUG, "After wait for reader %C -- Count = %d at %T\n",
               (tojoin ? "startup" : "shutdown"), pms.current_count));
    if (tojoin != (pms.current_count > 0)) {
      ACE_ERROR((LM_ERROR, "Data reader %C not detected at %T\n",
                 (tojoin ? "startup" : "shutdown")));
    }
  }
  ws->detach_condition(condition);
  return success;
}

bool wait_helper(DDS::WaitSet_var ws, DDS::StatusKind kind)
{
  DDS::ConditionSeq active;
  DDS::Duration_t infinite = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };

  DDS::ReturnCode_t ret = ws->wait(active, infinite);
  if (ret != DDS::RETCODE_OK) {
    const char* kind_str = kind == DDS::INCONSISTENT_TOPIC_STATUS ? "INCONSISTENT_TOPIC_STATUS" :
      (kind == DDS::OFFERED_INCOMPATIBLE_QOS_STATUS ? "OFFERED_INCOMPATIBLE_QOS_STATUS" :
       "PUBLICATION_MATCHED_STATUS");
    ACE_ERROR((LM_ERROR, "ERROR wait_helper - %C condition wait failed: %C\n",
               kind_str, OpenDDS::DCPS::retcode_to_string(ret)));
    return false;
  }
  return true;
}

bool wait_inconsistent_topic(DDS::WaitSet_var ws, DDS::Topic_var topic)
{
  while (true) {
    DDS::InconsistentTopicStatus status;
    DDS::ReturnCode_t ret = topic->get_inconsistent_topic_status(status);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR wait_inconsistent_topic - get_inconsistent_topic_status failed: %C\n",
                 OpenDDS::DCPS::retcode_to_string(ret)));
      return false;
    }

    if (status.total_count > 0) {
      return true;
    }
    if (!wait_helper(ws, DDS::INCONSISTENT_TOPIC_STATUS)) {
      return false;
    }
  }
}

bool wait_offered_incompatible_qos(DDS::WaitSet_var ws, DDS::DataWriter_var dw)
{
  while (true) {
    DDS::OfferedIncompatibleQosStatus status;
    DDS::ReturnCode_t ret = dw->get_offered_incompatible_qos_status(status);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR wait_offered_incompatible_qos - get_offered_incompatible_qos_status failed: %C\n",
                 OpenDDS::DCPS::retcode_to_string(ret)));
      return false;
    }

    if (status.total_count > 0) {
      return true;
    }
    if (!wait_helper(ws, DDS::OFFERED_INCOMPATIBLE_QOS_STATUS)) {
      return false;
    }
  }
}

bool wait_publication_matched(DDS::WaitSet_var ws, DDS::DataWriter_var dw)
{
  while (true) {
    DDS::PublicationMatchedStatus status;
    DDS::ReturnCode_t ret = dw->get_publication_matched_status(status);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR wait_publication_matched - get_publication_matched_status failed: %C\n",
                 OpenDDS::DCPS::retcode_to_string(ret)));
      return false;
    }

    if (status.total_count > 0) {
      return true;
    }
    if (!wait_helper(ws, DDS::PUBLICATION_MATCHED_STATUS)) {
      return false;
    }
  }
}

bool wait_requested_incompatible_qos(DDS::WaitSet_var ws, DDS::DataReader_var dr)
{
  while (true) {
    DDS::RequestedIncompatibleQosStatus status;
    DDS::ReturnCode_t ret = dr->get_requested_incompatible_qos_status(status);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR: wait_requested_incompatible_qos - get_requested_incompatible_qos_status failed: %C\n",
                 OpenDDS::DCPS::retcode_to_string(ret)));
      return false;
    }

    if (status.total_count > 0) {
      return true;
    }
    if (!wait_helper(ws, DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS)) {
      return false;
    }
  }
}

bool wait_subscription_matched(DDS::WaitSet_var ws, DDS::DataReader_var dr)
{
  while (true) {
    DDS::SubscriptionMatchedStatus status;
    DDS::ReturnCode_t ret = dr->get_subscription_matched_status(status);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR wait_subscription_matched - get_subscription_matched_status failed: %C\n",
                 OpenDDS::DCPS::retcode_to_string(ret)));
      return false;
    }

    if (status.total_count > 0) {
      return true;
    }
    if (!wait_helper(ws, DDS::SUBSCRIPTION_MATCHED_STATUS)) {
      return false;
    }
  }
}

bool check_inconsistent_topic_status(Topic_var topic)
{
  InconsistentTopicStatus status;
  const ReturnCode_t retcode = topic->get_inconsistent_topic_status(status);
  if (retcode != DDS::RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: check_inconsistent_topic_status - get_inconsistent_topic_status failed: %C\n",
               OpenDDS::DCPS::retcode_to_string(retcode)));
    return false;
  } else if (expect_inconsistent_topic != (status.total_count > 0)) {
    ACE_ERROR((LM_ERROR, "ERROR: check_inconsistent_topic_status - inconsistent topic count is %d\n", status.total_count));
    return false;
  }
  return true;
}

bool check_offered_incompatible_qos_status(DDS::DataWriter_var dw, const std::string& type)
{
  if (expect_incompatible_qos) {
    DDS::OfferedIncompatibleQosStatus qos_status;
    const DDS::ReturnCode_t ret = dw->get_offered_incompatible_qos_status(qos_status);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR: check_offered_incompatible_qos_status - get_offer_incompatible_qos_status failed: %C\n",
                 OpenDDS::DCPS::retcode_to_string(ret)));
      return false;
    }
    if (qos_status.policies.length() != 1 ||
        qos_status.policies[0].policy_id != DDS::DATA_REPRESENTATION_QOS_POLICY_ID) {
      ACE_ERROR((LM_ERROR, "ERROR: check_offered_incompatible_qos_status - %C QoS that was expected to fail did not.\n", type.c_str()));
      return false;
    }
  }
  return true;
}

bool check_requested_incompatible_qos_status(DDS::DataReader_var dr, const std::string& type)
{
  if (expect_incompatible_qos) {
    DDS::RequestedIncompatibleQosStatus qos_status;
    const DDS::ReturnCode_t ret = dr->get_requested_incompatible_qos_status(qos_status);
    if (ret != DDS::RETCODE_OK) {
      ACE_ERROR((LM_ERROR, "ERROR: check_requested_incompatible_qos_status - get_requested_incompatible_qos_status failed: %C\n",
                 OpenDDS::DCPS::retcode_to_string(ret)));
      return false;
    }
    if (qos_status.policies.length() != 1 ||
        qos_status.policies[0].policy_id != DDS::DATA_REPRESENTATION_QOS_POLICY_ID) {
      ACE_ERROR((LM_ERROR, "ERROR: check_requested_incompatible_qos_status - %C QoS that was expected to fail did not.\n", type.c_str()));
      return false;
    }
  }
  return true;
}

void append(PropertySeq& props, const char* name, const char* value)
{
  const Property_t prop = { name, value, false /*propagate*/ };
  const unsigned int len = props.length();
  props.length(len + 1);
  props[len] = prop;
}

void create_participant(const DomainParticipantFactory_var& dpf, DomainParticipant_var& dp) {
  DomainParticipantQos part_qos;
  dpf->get_default_participant_qos(part_qos);

#if defined(OPENDDS_SECURITY)
  if (TheServiceParticipant->get_security()) {
    using namespace DDS::Security::Properties;
    PropertySeq& props = part_qos.property.value;

    append(props, AuthIdentityCA, "file:../../security/certs/identity/identity_ca_cert.pem");
    append(props, AuthIdentityCertificate, "file:../../security/certs/identity/test_participant_02_cert.pem");
    append(props, AuthPrivateKey, "file:../../security/certs/identity/test_participant_02_private_key.pem");
    append(props, AccessPermissionsCA, "file:../../security/certs/permissions/permissions_ca_cert.pem");
    append(props, AccessGovernance, "file:./governance.xml.p7s");
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
