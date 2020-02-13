#include "TestMsgTypeSupportImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/WaitSet.h"

#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsCoreTypeSupportImpl.h"
#include "dds/DCPS/GuidConverter.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "tests/Utils/StatusMatching.h"
#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_unistd.h"

class TestConfig {
public:
  static void set(int base) {
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("%P TestConfig::set base=%d\n"), base));
    topic_data_ = base;
    topic_data2_ = base + 1;
    participant_user_data_ = base + 2;
    participant_user_data2_ = base + 3;
    data_writer_user_data_ = base + 4;
    data_writer_user_data2_ = base + 5;
    data_reader_user_data_ = base + 6;
    data_reader_user_data2_ = base + 7;
  }

  static int TOPIC_DATA() {
    return topic_data_;
  }

  static int TOPIC_DATA2() {
    return topic_data2_;
  }

  static int PARTICIPANT_USER_DATA() {
    return participant_user_data_;
  }

  static int PARTICIPANT_USER_DATA2() {
    return participant_user_data2_;
  }

  static int DATA_WRITER_USER_DATA() {
    return data_writer_user_data_;
  }

  static int DATA_WRITER_USER_DATA2() {
    return data_writer_user_data2_;
  }

  static int DATA_READER_USER_DATA() {
    return data_reader_user_data_;
  }

  static int DATA_READER_USER_DATA2() {
    return data_reader_user_data2_;
  }

private:
  static int topic_data_;
  static int topic_data2_;
  static int participant_user_data_;
  static int participant_user_data2_;
  static int data_writer_user_data_;
  static int data_writer_user_data2_;
  static int data_reader_user_data_;
  static int data_reader_user_data2_;
};

int TestConfig::topic_data_ = 1;
int TestConfig::topic_data2_ = 2;
int TestConfig::participant_user_data_ = 3;
int TestConfig::participant_user_data2_ = 4;
int TestConfig::data_writer_user_data_ = 5;
int TestConfig::data_writer_user_data2_ = 6;
int TestConfig::data_reader_user_data_ = 7;
int TestConfig::data_reader_user_data2_ = 8;

using namespace DDS;
using OpenDDS::DCPS::TransportConfig_rch;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;
using OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC;
using OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC;
using OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC;

void cleanup(const DomainParticipantFactory_var& dpf,
             const DomainParticipant_var& dp)
{
  if (!dpf || !dp) return;

  ReturnCode_t ret = dp->delete_contained_entities();
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: %P delete_contained_entities() returned %d\n",
               ret));
  }

  ret = dpf->delete_participant(dp);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: %P delete_participant() returned %d\n", ret));
  }
}

void set_qos(OctetSeq& qos, CORBA::Octet value)
{
  qos.length(1);
  qos[0] = value;
}

bool read_participant_bit(const Subscriber_var& bit_sub,
                          const DomainParticipant_var& dp,
                          const OpenDDS::DCPS::RepoId& other_dp_repo_id,
                          int user_data)
{
  OpenDDS::DCPS::Discovery_rch disc =
    TheServiceParticipant->get_discovery(dp->get_domain_id());
  OpenDDS::DCPS::DomainParticipantImpl* dp_impl =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(dp.in());

  if (!dp_impl) {
    ACE_ERROR_RETURN((LM_ERROR, "ERROR: could not obtain DomainParticipantImpl\n"), false);
  }

  DataReader_var dr = bit_sub->lookup_datareader(BUILT_IN_PARTICIPANT_TOPIC);
  ReadCondition_var rc = dr->create_readcondition(ANY_SAMPLE_STATE,
                                                  ANY_VIEW_STATE,
                                                  ALIVE_INSTANCE_STATE);
  WaitSet_var waiter = new WaitSet;
  waiter->attach_condition(rc);
  ConditionSeq activeConditions;
  Duration_t forever = { DURATION_INFINITE_SEC,
                         DURATION_INFINITE_NSEC };
  ReturnCode_t result = waiter->wait(activeConditions, forever);
  waiter->detach_condition(rc);
  if (result != RETCODE_OK) {
    ACE_ERROR_RETURN((LM_ERROR, "ERROR: %P could not wait for condition: %d\n", result), false);
  }

  ParticipantBuiltinTopicDataDataReader_var part_bit =
    ParticipantBuiltinTopicDataDataReader::_narrow(dr);

  ParticipantBuiltinTopicDataSeq data;
  SampleInfoSeq infos;
  ReturnCode_t ret =
    part_bit->read_w_condition(data, infos, LENGTH_UNLIMITED, rc);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: %P could not read participant BIT: %d\n", ret));
    return false;
  }

  bool found_other_dp = false;
  int num_valid = 0;

  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (infos[i].valid_data) {
      ++num_valid;
      OpenDDS::DCPS::RepoId repo_id =
        disc->bit_key_to_repo_id(dp_impl,
                                 OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC,
                                 data[i].key);

      OpenDDS::DCPS::GuidConverter converter(repo_id);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("%P ")
                 ACE_TEXT("Read Participant BIT GUID=%C handle=%d\n"),
                 OPENDDS_STRING(converter).c_str(), infos[i].instance_handle));

      if (repo_id == other_dp_repo_id) {
        if (data[i].user_data.value.length() != 1) {
          ACE_ERROR_RETURN((LM_ERROR,
                            "ERROR: %P participant[%d] user data length %d "
                            "not expected length of 1\n",
                            i,
                            data[i].user_data.value.length()),
                           false);
        }

        if (data[i].user_data.value[0] != user_data) {
          ACE_ERROR_RETURN((LM_ERROR,
                            "ERROR: %P participant[%d] user data value %d "
                            "not expected value %d\n",
                            i,
                            data[i].user_data.value[0],
                            user_data),
                           false);
        }

        found_other_dp = true;
      }
    }
  }

  if (num_valid != 1) {
    ACE_ERROR((LM_ERROR, "ERROR: %P expected to discover 1 other participant, found %d\n", data.length ()));
  }

  part_bit->return_loan(data, infos);

  if (!found_other_dp) {
    ACE_ERROR_RETURN((LM_ERROR, "ERROR: %P did not find expected participant\n"), false);
  }

  return true;
}

DataWriter_var create_data_writer(const DomainParticipant_var& dp2)
{
  TypeSupport_var ts = new TestMsgTypeSupportImpl;

  if (ts->register_type(dp2, "") != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: %P failed to register type support\n"));
    return 0;
  }

  TopicQos topic_qos;
  dp2->get_default_topic_qos(topic_qos);
  set_qos(topic_qos.topic_data.value, TestConfig::TOPIC_DATA());
  CORBA::String_var type_name = ts->get_type_name();
  Topic_var topic = dp2->create_topic("Movie Discussion List",
                                      type_name,
                                      topic_qos,
                                      0,
                                      DEFAULT_STATUS_MASK);

  if (!topic) {
    ACE_ERROR((LM_ERROR, "ERROR: %P failed to create topic\n"));
    return 0;
  }

  Publisher_var pub = dp2->create_publisher(PUBLISHER_QOS_DEFAULT,
                                            0,
                                            DEFAULT_STATUS_MASK);

  if (!pub) {
    ACE_ERROR((LM_ERROR, "ERROR: %P failed to create publisher\n"));
    return 0;
  }

  DataWriterQos dw_qos;
  pub->get_default_datawriter_qos(dw_qos);
  set_qos(dw_qos.user_data.value, TestConfig::DATA_WRITER_USER_DATA());

  DataWriter_var dw = pub->create_datawriter(topic,
                                             dw_qos,
                                             0,
                                             DEFAULT_STATUS_MASK);

  if (!dw) {
    ACE_ERROR((LM_ERROR, "ERROR: %P failed to create data writer\n"));
    return 0;
  }
  return dw;
}

void recreate_data_writer_and_topic(DataWriter_var& dw, const DataReader_var& dr)
{
  DataWriterQos dw_qos;
  dw->get_qos(dw_qos);

  Topic_var topic = dw->get_topic();
  TopicQos topic_qos;
  topic->get_qos(topic_qos);
  CORBA::String_var topic_name = topic->get_name(),
    type_name = topic->get_type_name();

  Publisher_var pub = dw->get_publisher();
  DomainParticipant_var dp = pub->get_participant();
  pub->delete_datawriter(dw);
  dw = 0;

  dp->delete_topic(topic);
  topic = 0;

  // Wait until the data reader is not associated with the writer.
  Utils::wait_match(dr, 0);

  topic = dp->create_topic(topic_name, type_name, topic_qos, 0, 0);
  if (!topic) {
    ACE_ERROR((LM_ERROR, "ERROR: %P failed to re-create topic\n"));
    return;
  }

  dw = pub->create_datawriter(topic, dw_qos, 0, 0);
  if (!dw) {
    ACE_ERROR((LM_ERROR, "ERROR: %P failed to re-create data writer\n"));
  }
}

DataReader_var create_data_reader(const DomainParticipant_var& dp)
{
  TypeSupport_var ts = new TestMsgTypeSupportImpl;

  if (ts->register_type(dp, "") != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: %P failed to register type support\n"));
    return 0;
  }

  TopicQos topic_qos;
  dp->get_default_topic_qos(topic_qos);
  set_qos(topic_qos.topic_data.value, TestConfig::TOPIC_DATA());
  CORBA::String_var type_name = ts->get_type_name();
  Topic_var topic = dp->create_topic("Movie Discussion List",
                                     type_name,
                                     topic_qos,
                                     0,
                                     DEFAULT_STATUS_MASK);

  if (!topic) {
    ACE_ERROR((LM_ERROR, "ERROR: %P failed to create topic\n"));
    return 0;
  }

  Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                             0,
                                             DEFAULT_STATUS_MASK);

  if (!sub) {
    ACE_ERROR((LM_ERROR, "ERROR: %P failed to create subscriber\n"));
    return 0;
  }

  DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
  set_qos(dr_qos.user_data.value, TestConfig::DATA_READER_USER_DATA());

  DataReader_var dr = sub->create_datareader(topic,
                                             dr_qos,
                                             0,
                                             DEFAULT_STATUS_MASK);

  if (!dr) {
    ACE_ERROR((LM_ERROR, "ERROR: %P failed to create data reader\n"));
    return 0;
  }
  return dr;
}

bool read_publication_bit(const Subscriber_var& bit_sub,
                          const DomainParticipant_var& subscriber,
                          const OpenDDS::DCPS::RepoId& publisher_repo_id,
                          InstanceHandle_t& handle,
                          int user_data,
                          int topic_data,
                          int expected_min,
                          int expected_max)
{
  bool ignored_publication = expected_max == 0;
  OpenDDS::DCPS::Discovery_rch disc =
    TheServiceParticipant->get_discovery(subscriber->get_domain_id());
  OpenDDS::DCPS::DomainParticipantImpl* subscriber_impl =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(subscriber.in());

  if (!subscriber_impl) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: %P read_publication_bit: ")
      ACE_TEXT("Failed to obtain DomainParticipantImpl\n")),
      false);
  }

  DataReader_var dr = bit_sub->lookup_datareader(BUILT_IN_PUBLICATION_TOPIC);
  if (!ignored_publication) {
    ReadCondition_var rc = dr->create_readcondition(ANY_SAMPLE_STATE,
                                                    ANY_VIEW_STATE,
                                                    ALIVE_INSTANCE_STATE);
    WaitSet_var waiter = new WaitSet;
    waiter->attach_condition(rc);
    ConditionSeq activeConditions;
    Duration_t forever = { DURATION_INFINITE_SEC,
                           DURATION_INFINITE_NSEC };
    ReturnCode_t result = waiter->wait(activeConditions, forever);
    waiter->detach_condition(rc);
    if (result != RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
                 "ERROR: %P (publication BIT) could not wait for condition: %d\n", result));
      return false;
    }
  } else {
    ACE_OS::sleep(1);
  }

  PublicationBuiltinTopicDataDataReader_var pub_bit =
    PublicationBuiltinTopicDataDataReader::_narrow(dr);

  PublicationBuiltinTopicDataSeq data;
  SampleInfoSeq infos;
  ReturnCode_t ret =
    pub_bit->read(data, infos, LENGTH_UNLIMITED,
                  ANY_SAMPLE_STATE, ANY_VIEW_STATE, ALIVE_INSTANCE_STATE);
  if (ignored_publication && (ret != RETCODE_NO_DATA)) {
    ACE_ERROR((LM_ERROR, "ERROR: %P could not read ignored publication BIT: %d\n",
               ret));
    return false;
  } else if (ret != RETCODE_OK && ret != RETCODE_NO_DATA) {
    ACE_ERROR((LM_ERROR, "ERROR: %P could not read publication BIT: %d\n", ret));
    return false;
  }

  int num_valid = 0;
  bool found_publisher = false;
  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (infos[i].valid_data) {
      ++num_valid;

      OpenDDS::DCPS::RepoId repo_id =
        disc->bit_key_to_repo_id(subscriber_impl,
                                 OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC,
                                 data[i].participant_key);

      OpenDDS::DCPS::GuidConverter converter(repo_id);

      ACE_DEBUG((LM_DEBUG,
                 "%P Read Publication BIT with key: %x %x %x and handle %d\n"
                 "\tParticipant's GUID=%C\n\tTopic: %C\tType: %C\n",
                 data[i].key.value[0], data[i].key.value[1],
                 data[i].key.value[2], infos[i].instance_handle,
                 OPENDDS_STRING(converter).c_str (), data[i].topic_name.in(),
                 data[i].type_name.in()));

      if (repo_id == publisher_repo_id) {
        found_publisher = true;
        if (data[i].user_data.value.length() != 1) {
          ACE_ERROR_RETURN((LM_ERROR,
                            "ERROR: %P publication [%d] user data length %d "
                            "not expected length of 1\n",
                            i,
                            data[i].user_data.value.length()),
                           false);
        }
        if (data[i].topic_data.value.length() != 1) {
          ACE_ERROR_RETURN((LM_ERROR,
                            "ERROR: %P publication [%d] topic data length %d "
                            "not expected length of 1\n",
                            i,
                            data[i].topic_data.value.length()),
                           false);
        }
        if (data[i].user_data.value[0] != user_data) {
          ACE_ERROR_RETURN((LM_ERROR,
                            "ERROR: %P publication [%d] user data value %d "
                            "not expected value %d\n",
                            i,
                            data[i].user_data.value[0],
                            user_data),
                           false);
        }
        if (data[i].topic_data.value[0] != topic_data) {
          ACE_ERROR_RETURN((LM_ERROR,
                            "ERROR: %P publication [%d] topic data value %d "
                            "not expected value %d\n",
                            i,
                            data[i].topic_data.value[0],
                            topic_data),
                           false);
        }
      }
      handle = infos[i].instance_handle;
    }
  }

  if (num_valid < expected_min || num_valid > expected_max) {
    ACE_ERROR_RETURN((LM_ERROR, "ERROR: %P expected %d to %d discovered "
                      "publications, found %d\n",
                      expected_min, expected_max, num_valid), false);
  }
  if (expected_min > 0 && !found_publisher) {
    ACE_ERROR_RETURN((LM_ERROR, "ERROR: %P did not find expected publication\n"), false);
  }

  return true;
}

bool read_subscription_bit(const Subscriber_var& bit_sub,
                           const DomainParticipant_var& publisher,
                           const OpenDDS::DCPS::RepoId& subscriber_repo_id,
                           InstanceHandle_t& handle,
                           int user_data,
                           int topic_data,
                           bool ignored_subscription = false)
{
  OpenDDS::DCPS::Discovery_rch disc =
    TheServiceParticipant->get_discovery(publisher->get_domain_id());
  OpenDDS::DCPS::DomainParticipantImpl* publisher_impl =
    dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(publisher.in());

  if (!publisher_impl) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: %P read_subscription_bit: ")
      ACE_TEXT("Failed to obtain DomainParticipantImpl\n")),
      false);
  }

  DataReader_var dr = bit_sub->lookup_datareader(BUILT_IN_SUBSCRIPTION_TOPIC);
  if (!ignored_subscription) {
    ReadCondition_var rc = dr->create_readcondition(ANY_SAMPLE_STATE,
                                                    ANY_VIEW_STATE,
                                                    ALIVE_INSTANCE_STATE);
    WaitSet_var waiter = new WaitSet;
    waiter->attach_condition(rc);
    ConditionSeq activeConditions;
    Duration_t forever = { DURATION_INFINITE_SEC,
                           DURATION_INFINITE_NSEC };
    ReturnCode_t result = waiter->wait(activeConditions, forever);
    waiter->detach_condition(rc);
    if (result != RETCODE_OK) {
      ACE_ERROR((LM_ERROR,
        "ERROR: %P (subscription BIT) could not wait for condition: %d\n", result));
      return false;
    }
  } else {
    ACE_OS::sleep(1);
  }
  SubscriptionBuiltinTopicDataDataReader_var pub_bit =
    SubscriptionBuiltinTopicDataDataReader::_narrow(dr);

  SubscriptionBuiltinTopicDataSeq data;
  SampleInfoSeq infos;
  ReturnCode_t ret =
    pub_bit->read(data, infos, LENGTH_UNLIMITED,
                  ANY_SAMPLE_STATE, ANY_VIEW_STATE, ALIVE_INSTANCE_STATE);
  if (ignored_subscription && (ret != RETCODE_NO_DATA)) {
    ACE_ERROR((LM_ERROR, "ERROR: %P could not read ignored subscription BIT: %d\n",
               ret));
    return false;
  } else if (ret != RETCODE_OK && ret != RETCODE_NO_DATA) {
    ACE_ERROR((LM_ERROR, "ERROR: %P could not read subscription BIT: %d\n", ret));
    return false;
  }

  int num_valid = 0;
  bool found_subscriber = false;
  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (infos[i].valid_data) {
      ++num_valid;

      OpenDDS::DCPS::RepoId repo_id =
        disc->bit_key_to_repo_id(publisher_impl,
                                 OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC,
                                 data[i].participant_key);

      OpenDDS::DCPS::GuidConverter converter(repo_id);

      ACE_DEBUG((LM_DEBUG,
                 "%P Read Subscription BIT with key: %x %x %x and handle %d\n"
                 "\tParticipant's GUID=%C\n\tTopic: %C\tType: %C\n",
                 data[i].key.value[0], data[i].key.value[1],
                 data[i].key.value[2], infos[i].instance_handle,
                 OPENDDS_STRING(converter).c_str (), data[i].topic_name.in(),
                 data[i].type_name.in()));
      if (repo_id == subscriber_repo_id) {
        found_subscriber = true;
        if (data[i].user_data.value.length() != 1) {
          ACE_ERROR_RETURN((LM_ERROR,
                            "ERROR: %P subscription [%d] user data length %d "
                            "not expected length of 1\n",
                            i,
                            data[i].user_data.value.length()),
                           false);
        }
        if (data[i].topic_data.value.length() != 1) {
          ACE_ERROR_RETURN((LM_ERROR,
                            "ERROR: %P subscription [%d] topic data length %d "
                            "not expected length of 1\n",
                            i,
                            data[i].topic_data.value.length()),
                           false);
        }
        if (data[i].user_data.value[0] != user_data) {
          ACE_ERROR_RETURN((LM_ERROR,
                            "ERROR: %P subscription [%d] user data value %d "
                            "not expected value %d\n",
                            i,
                            data[i].user_data.value[0],
                            user_data),
                           false);
        }
        if (data[i].topic_data.value[0] != topic_data) {
          ACE_ERROR_RETURN((LM_ERROR,
                            "ERROR: %P subscription [%d] topic data value %d "
                            "not expected value %d\n",
                            i,
                            data[i].topic_data.value[0],
                            topic_data),
                           false);
        }
      }
      handle = infos[i].instance_handle;
    }
  }
  if (ignored_subscription) {
    if (num_valid != 0) {
      ACE_ERROR_RETURN((LM_ERROR, "ERROR: %P expected 0 discovered "
                        "subscriptions, found %d\n",
                        num_valid), false);
    }
  }
  else {
    if (num_valid != 1) {
      ACE_ERROR_RETURN((LM_ERROR, "ERROR: %P expected 1 discovered "
                        "subscriptions, found %d\n",
                        num_valid), false);
    }
    if (!found_subscriber) {
      ACE_ERROR_RETURN((LM_ERROR, "ERROR: %P did not find expected subscription\n"), false);
    }
  }

  return true;
}

bool check_discovered_participants(DomainParticipant_var& dp,
                                   InstanceHandle_t& handle)
{
  InstanceHandle_t my_handle    = dp->get_instance_handle();

  DDS::InstanceHandleSeq part_handles;
  DDS::ReturnCode_t stat = dp->get_discovered_participants(part_handles);
  while (stat == RETCODE_NO_DATA || part_handles.length() == 0) {
    ACE_OS::sleep(1);
    stat = dp->get_discovered_participants(part_handles);
  }

  if (stat == RETCODE_OK) {
    CORBA::ULong len = part_handles.length();
    if (len != 1) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: %P expected to discover")
                                  ACE_TEXT("one other participant handle but ")
                                  ACE_TEXT("found %d\n"), len), false);
    } else if (part_handles[0] == my_handle) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: %P discovered own ")
                                  ACE_TEXT("participant handle\n")), false);
    } else {
      DDS::ParticipantBuiltinTopicData data;
      dp->get_discovered_participant_data(data, part_handles[0]);
      OpenDDS::DCPS::Discovery_rch disc =
          TheServiceParticipant->get_discovery(dp->get_domain_id());
      OpenDDS::DCPS::DomainParticipantImpl* dp_impl =
          dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(dp.in());

      if (!dp_impl) {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: %P check_discovered_participants: ")
          ACE_TEXT("Failed to obtain DomainParticipantImpl\n")),
          false);
      }

      OpenDDS::DCPS::RepoId repo_id = disc->bit_key_to_repo_id(
          dp_impl,
          OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC,
          data.key);
      if (dp_impl->id_to_handle(repo_id) != part_handles[0]) {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: %P discovered participant ")
                                    ACE_TEXT("BIT key could not be converted ")
                                    ACE_TEXT("to repo id, then handle\n")),
                         false);
      }
      handle = part_handles[0];
      {
        OpenDDS::DCPS::GuidConverter converter1(dp_impl->get_id ());
        OpenDDS::DCPS::GuidConverter converter2(repo_id);
        ACE_DEBUG ((LM_DEBUG,
                    ACE_TEXT("%P ")
                    ACE_TEXT("%C discovered %C\n"),
                    OPENDDS_STRING(converter1).c_str(),
                    OPENDDS_STRING(converter2).c_str()));
      }
    }
  }
  return (stat == RETCODE_OK);
}

bool run_test(DomainParticipant_var& dp_sub,
              DomainParticipant_var& dp_pub)
{
  OpenDDS::DCPS::RepoId sub_repo_id, pub_repo_id;

  {
    OpenDDS::DCPS::DomainParticipantImpl* dp_impl =
      dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(dp_sub.in());

    if (!dp_impl) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: %P run_test: ")
        ACE_TEXT("Failed to obtain DomainParticipantImpl for sub\n")),
        false);
    }

    sub_repo_id = dp_impl->get_id ();
    OpenDDS::DCPS::GuidConverter converter(sub_repo_id);
    ACE_DEBUG ((LM_DEBUG,
                ACE_TEXT("%P ")
                ACE_TEXT("Sub Domain Participant GUID=%C\n"),
                OPENDDS_STRING(converter).c_str()));
  }

  {
    OpenDDS::DCPS::DomainParticipantImpl* dp_impl =
      dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(dp_pub.in());

    if (!dp_impl) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: %P run_test: ")
        ACE_TEXT("Failed to obtain DomainParticipantImpl for pub\n")),
        false);
    }

    pub_repo_id = dp_impl->get_id ();
    OpenDDS::DCPS::GuidConverter converter(pub_repo_id);
    ACE_DEBUG ((LM_DEBUG,
                ACE_TEXT("%P ")
                ACE_TEXT("Pub Domain Participant GUID=%C\n"),
                OPENDDS_STRING(converter).c_str()));
  }

  // If we are running with an rtps_udp transport, it can't be shared between
  // participants.
  TransportConfig_rch cfg = TheTransportRegistry->get_config("dp1");
  if (!cfg.is_nil()) {
    TheTransportRegistry->bind_config(cfg, dp_sub);
  }
  cfg = TheTransportRegistry->get_config("dp2");
  if (!cfg.is_nil()) {
    TheTransportRegistry->bind_config(cfg, dp_pub);
  }

  Subscriber_var bit_sub = dp_sub->get_builtin_subscriber();

  if (!read_participant_bit(bit_sub, dp_sub, pub_repo_id, TestConfig::PARTICIPANT_USER_DATA())) {
    return false;
  }

  // Each domain participant's handle to the other
  InstanceHandle_t dp_sub_ih, dp_pub_ih;
  InstanceHandle_t pub_ih, sub_ih, ig_ih;

  if (!(check_discovered_participants(dp_sub, dp_pub_ih) &&
        check_discovered_participants(dp_pub, dp_sub_ih)))
  {
    return false;
  }

  DataWriter_var dw = create_data_writer(dp_pub);
  if (!dw) {
    ACE_ERROR((LM_ERROR, "ERROR: %P could not create Data Writer (participant 2)\n"));
    return false;
  }

  if (!read_publication_bit(bit_sub, dp_sub, pub_repo_id, pub_ih, TestConfig::DATA_WRITER_USER_DATA(), TestConfig::TOPIC_DATA(), 1, 1)) {
    return false;
  }

  DataReader_var dr = create_data_reader(dp_sub);
  if (!dr) {
    ACE_ERROR((LM_ERROR, "ERROR: %P could not create Data Reader (participant 1)\n"));
    return false;
  }
  if (!read_subscription_bit(dp_pub->get_builtin_subscriber(), dp_pub, sub_repo_id, sub_ih, TestConfig::DATA_READER_USER_DATA(), TestConfig::TOPIC_DATA())) {
    return false;
  }

  // Wait for the reader to associate with the writer.
  Utils::wait_match(dw, 1, Utils::GTE);

  // Remove the writer and its topic, then re-create them.  The writer's
  // participant should still have discovery info about the reader so that
  // the association between the new writer and old reader can be established.
  recreate_data_writer_and_topic(dw, dr);

  // Wait for the reader to associate with the writer.
  Utils::wait_match(dw, 1, Utils::GTE);

  // The new writer is associated with the reader, but the reader may still
  // also be associated with the old writer.
  Utils::wait_match(dr, 1);

  // Get the new instance handle as pub_ih
  if (!read_publication_bit(bit_sub, dp_sub, pub_repo_id, pub_ih, TestConfig::DATA_WRITER_USER_DATA(), TestConfig::TOPIC_DATA(), 1, 2)) {
    return false;
  }

  TestMsgDataWriter_var tmdw = TestMsgDataWriter::_narrow(dw);
  const TestMsg msg = {42};
  tmdw->write(msg, HANDLE_NIL);

  ReadCondition_var rc = dr->create_readcondition(ANY_SAMPLE_STATE,
                                                  ANY_VIEW_STATE,
                                                  ALIVE_INSTANCE_STATE);
  WaitSet_var waiter = new WaitSet;
  waiter->attach_condition(rc);
  ConditionSeq activeConditions;
  const Duration_t timeout = { 90, 0 };
  ReturnCode_t result = waiter->wait(activeConditions, timeout);
  waiter->detach_condition(rc);
  if (result != RETCODE_OK) {
    ACE_ERROR((LM_ERROR,
      "ERROR: %P TestMsg reader could not wait for condition: %d\n", result));
    return false;
  }

  TestMsgDataReader_var tmdr = TestMsgDataReader::_narrow(dr);

  TestMsgSeq data;
  SampleInfoSeq infos;
  ReturnCode_t ret = tmdr->read_w_condition(data, infos, LENGTH_UNLIMITED, rc);
  if (ret != RETCODE_OK) {
    ACE_ERROR((LM_ERROR, "ERROR: %P could not read TestMsg: %d\n", ret));
    return false;
  }

  bool ok = false;
  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (infos[i].valid_data) {
      ok = true;
      ACE_DEBUG((LM_DEBUG, "%P Read data sample: %d\n", data[i].value));
    }
  }

  if (!ok) {
    ACE_ERROR((LM_ERROR, "ERROR: %P no valid data from TestMsg data reader\n"));
  }

  // Change dp qos
  {
    DomainParticipantQos dp_qos;
    dp_pub->get_qos(dp_qos);
    set_qos(dp_qos.user_data.value, TestConfig::PARTICIPANT_USER_DATA2());
    dp_pub->set_qos(dp_qos);
  }
  // Change dw qos
  {
    DataWriterQos dw_qos;
    dw->get_qos(dw_qos);
    set_qos(dw_qos.user_data.value, TestConfig::DATA_WRITER_USER_DATA2());
    dw->set_qos(dw_qos);
  }
  // Change dr qos
  {
    DataReaderQos dr_qos;
    dr->get_qos(dr_qos);
    set_qos(dr_qos.user_data.value, TestConfig::DATA_READER_USER_DATA2());
    dr->set_qos(dr_qos);
  }
  // Wait for propagation
  ACE_OS::sleep(3);
  if (!read_participant_bit(bit_sub, dp_sub, pub_repo_id, TestConfig::PARTICIPANT_USER_DATA2())) {
    return false;
  }
  if (!read_publication_bit(bit_sub, dp_sub, pub_repo_id, ig_ih, TestConfig::DATA_WRITER_USER_DATA2(), TestConfig::TOPIC_DATA(), 1, 1)) {
    return false;
  }
  if (!read_subscription_bit(dp_pub->get_builtin_subscriber(), dp_pub, sub_repo_id, ig_ih, TestConfig::DATA_READER_USER_DATA2(), TestConfig::TOPIC_DATA())) {
    return false;
  }

  // Set dw topic qos
  Topic_var topic = dw->get_topic();
  TopicQos topic_qos;
  topic->get_qos(topic_qos);
  set_qos(topic_qos.topic_data.value, TestConfig::TOPIC_DATA2());
  topic->set_qos(topic_qos);

  // Set dr topic qos
  TopicDescription_var topic_desc = dr->get_topicdescription();
  topic = Topic::_narrow(topic_desc);
  topic->get_qos(topic_qos);
  set_qos(topic_qos.topic_data.value, TestConfig::TOPIC_DATA2());
  topic->set_qos(topic_qos);

  // Wait for propagation
  ACE_OS::sleep(3);
  if (!read_publication_bit(bit_sub, dp_sub, pub_repo_id, ig_ih, TestConfig::DATA_WRITER_USER_DATA2(), TestConfig::TOPIC_DATA2(), 1, 1)) {
    return false;
  }
  if (!read_subscription_bit(dp_pub->get_builtin_subscriber(), dp_pub, sub_repo_id, ig_ih, TestConfig::DATA_READER_USER_DATA2(), TestConfig::TOPIC_DATA2())) {
    return false;
  }

  // Test ignore
  dp_sub->ignore_publication(pub_ih);
  if (!read_publication_bit(bit_sub, dp_sub, pub_repo_id, pub_ih, TestConfig::DATA_WRITER_USER_DATA2(), TestConfig::TOPIC_DATA2(), 0, 0)) {
    ACE_ERROR_RETURN((LM_ERROR,
                     ACE_TEXT("ERROR: %P Could not ignore publication\n")), false);
  }

  dp_pub->ignore_subscription(sub_ih);
  if (!read_subscription_bit(dp_pub->get_builtin_subscriber(), dp_pub, sub_repo_id, sub_ih, TestConfig::DATA_READER_USER_DATA2(), TestConfig::TOPIC_DATA2(), true)) {
    ACE_ERROR_RETURN((LM_ERROR,
                     ACE_TEXT("ERROR: %P Could not ignore subscription\n")), false);
  }

  dp_sub->ignore_participant(dp_pub_ih);
  InstanceHandleSeq handles;
  dp_sub->get_discovered_participants(handles);
  // Check that the handle is no longer in the sequence.
  for (CORBA::ULong i = 0; i < handles.length (); ++i) {
    if (handles[i] == dp_pub_ih) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %P Could not ignore participant\n")), false);

    }
  }

  return ok;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  bool ok = false;
  DomainParticipantFactory_var dpf;
  DomainParticipant_var dp_sub, dp_pub;
  try {
    dpf = TheParticipantFactoryWithArgs(argc, argv);
    dp_sub = dpf->create_participant(9, PARTICIPANT_QOS_DEFAULT,
                                     0, DEFAULT_STATUS_MASK);
    if (!dp_sub) {
      ACE_ERROR((LM_ERROR, "ERROR: %P could not create Sub Domain Participant\n"));

    } else {
      {
        // New scope.
        ACE_Arg_Shifter shifter (argc, argv);
        while (shifter.is_anything_left ()) {
          const ACE_TCHAR* x = shifter.get_the_parameter (ACE_TEXT("-value_base"));
          if (x != NULL) {
            TestConfig::set (ACE_OS::atoi (x));
          }

          shifter.consume_arg ();
        }
      }

      DomainParticipantQos dp_qos;
      dpf->get_default_participant_qos(dp_qos);
      set_qos(dp_qos.user_data.value, TestConfig::PARTICIPANT_USER_DATA());
      dp_pub = dpf->create_participant(9, dp_qos, 0, DEFAULT_STATUS_MASK);

      if (!dp_pub) {
        ACE_ERROR((LM_ERROR, "ERROR: %P could not create Domain Participant 2\n"));

      } else {
        ok = run_test(dp_sub, dp_pub);

        if (!ok) {
          ACE_ERROR((LM_ERROR, "ERROR: %P from run_test\n"));
          return -1;
        }
      }
    }
  } catch (const std::exception& e) {
    ACE_ERROR((LM_ERROR, "ERROR: %P Exception thrown: %C\n", e.what()));
    return -2;
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("ERROR: %P Exception thrown:");
    return -2;
  } catch (const OpenDDS::DCPS::Transport::Exception&) {
    ACE_ERROR((LM_ERROR, "ERROR: %P Transport exception thrown\n"));
    return -2;
  } catch (...) {
    ACE_ERROR((LM_ERROR, "ERROR: %P unknown exception thrown\n"));
    return -2;
  }

  ACE_DEBUG((LM_INFO, "%P Cleaning up test\n"));
  cleanup(dpf, dp_sub);
  ACE_OS::sleep(2);
  cleanup(dpf, dp_pub);
  TheServiceParticipant->shutdown();
  ACE_Thread_Manager::instance()->wait();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
