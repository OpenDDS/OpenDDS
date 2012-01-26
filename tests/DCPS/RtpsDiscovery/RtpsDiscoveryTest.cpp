#include "TestMsgTypeSupportImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/WaitSet.h"

#include "dds/DCPS/transport/framework/TransportExceptions.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include "dds/DdsDcpsInfrastructureC.h"

#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#include "dds/DCPS/transport/tcp/Tcp.h"
#endif

#include "model/Sync.h"

using namespace DDS;
using OpenDDS::DCPS::TransportConfig_rch;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;
using OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC;
using OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC;
using OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC;
using OpenDDS::Model::WriterSync;

void cleanup(const DomainParticipantFactory_var& dpf,
             const DomainParticipant_var& dp)
{
  if (!dpf || !dp) return;

  ReturnCode_t ret = dp->delete_contained_entities();
  if (ret != RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG, "ERROR: delete_contained_entities() returned %d\n",
               ret));
  }

  ret = dpf->delete_participant(dp);
  if (ret != RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG, "ERROR: delete_participant() returned %d\n", ret));
  }
}

void set_qos(OctetSeq& qos, CORBA::Octet value)
{
  qos.length(1);
  qos[0] = value;
}

bool read_participant_bit(const Subscriber_var& bit_sub,
                          int user_data)
{
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
    ACE_DEBUG((LM_DEBUG, "ERROR: could not wait for condition: %d\n", result));
    return false;
  }

  ParticipantBuiltinTopicDataDataReader_var part_bit =
    ParticipantBuiltinTopicDataDataReader::_narrow(dr);

  ParticipantBuiltinTopicDataSeq data;
  SampleInfoSeq infos;
  ReturnCode_t ret =
    part_bit->read_w_condition(data, infos, LENGTH_UNLIMITED, rc);
  if (ret != RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG, "ERROR: could not read participant BIT: %d\n", ret));
    return false;
  }

  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (infos[i].valid_data) {
      ACE_DEBUG((LM_DEBUG,
                 "Read Participant BIT with key: %x %x %x and handle %d\n",
                 data[i].key.value[0],
                 data[i].key.value[1],
                 data[i].key.value[2],
                 infos[i].instance_handle));

      if (data[i].user_data.value.length() != 1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          "ERROR particpant[%d] user data length %d "
                          "not expected length of 1\n",
                          i,
                          data[i].user_data.value.length()),
                         false);
      }

      if (i != data.length() - 1) {
        continue;
      }
      if (data[i].user_data.value[0] != user_data) {
        ACE_ERROR_RETURN((LM_ERROR,
                          "ERROR particpant[%d] user data value %d "
                          "not expected value %d\n",
                          i,
                          data[i].user_data.value[0],
                          user_data),
                         false);
      }
    }
  }

  part_bit->return_loan(data, infos);
  return true;
}

DataWriter_var create_data_writer(const DomainParticipant_var& dp2)
{
  TypeSupport_var ts = new TestMsgTypeSupportImpl;

  if (ts->register_type(dp2, "") != RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to register type support"));
    return 0;
  }

  TopicQos topic_qos;
  dp2->get_default_topic_qos(topic_qos);
  set_qos(topic_qos.topic_data.value, 1);
  CORBA::String_var type_name = ts->get_type_name();
  Topic_var topic = dp2->create_topic("Movie Discussion List",
                                      type_name,
                                      topic_qos,
                                      0,
                                      DEFAULT_STATUS_MASK);

  if (!topic) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to create topic"));
    return 0;
  }

  Publisher_var pub = dp2->create_publisher(PUBLISHER_QOS_DEFAULT,
                                            0,
                                            DEFAULT_STATUS_MASK);

  if (!pub) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to create publisher"));
    return 0;
  }

  DataWriterQos dw_qos;
  pub->get_default_datawriter_qos(dw_qos);
  set_qos(dw_qos.user_data.value, 2);

  DataWriter_var dw = pub->create_datawriter(topic,
                                             dw_qos,
                                             0,
                                             DEFAULT_STATUS_MASK);

  if (!dw) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to create data writer"));
    return 0;
  }
  return dw;
}

DataReader_var create_data_reader(const DomainParticipant_var& dp)
{
  TypeSupport_var ts = new TestMsgTypeSupportImpl;

  if (ts->register_type(dp, "") != RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to register type support"));
    return 0;
  }

  TopicQos topic_qos;
  dp->get_default_topic_qos(topic_qos);
  set_qos(topic_qos.topic_data.value, 1);
  CORBA::String_var type_name = ts->get_type_name();
  Topic_var topic = dp->create_topic("Movie Discussion List",
                                     type_name,
                                     topic_qos,
                                     0,
                                     DEFAULT_STATUS_MASK);

  if (!topic) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to create topic"));
    return 0;
  }

  Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                             0,
                                             DEFAULT_STATUS_MASK);

  if (!sub) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to create subscriber"));
    return 0;
  }

  DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);
  dr_qos.reliability.kind = RELIABLE_RELIABILITY_QOS;
  set_qos(dr_qos.user_data.value, 4);

  DataReader_var dr = sub->create_datareader(topic,
                                             dr_qos,
                                             0,
                                             DEFAULT_STATUS_MASK);

  if (!dr) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to create data reader"));
    return 0;
  }
  return dr;
}

bool read_publication_bit(const Subscriber_var& bit_sub,
                          InstanceHandle_t& handle,
                          int user_data,
                          int topic_data,
                          int num_expected = 1)
{
  DataReader_var dr = bit_sub->lookup_datareader(BUILT_IN_PUBLICATION_TOPIC);
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
    ACE_DEBUG((LM_DEBUG,
      "ERROR: (publication BIT) could not wait for condition: %d\n", result));
    return false;
  }

  PublicationBuiltinTopicDataDataReader_var pub_bit =
    PublicationBuiltinTopicDataDataReader::_narrow(dr);

  PublicationBuiltinTopicDataSeq data;
  SampleInfoSeq infos;
  ReturnCode_t ret =
    pub_bit->read_w_condition(data, infos, LENGTH_UNLIMITED, rc);
  if (ret != RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG, "ERROR: could not read publication BIT: %d\n", ret));
    return false;
  }

  int num_valid = 0;
  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (infos[i].valid_data) {
      ++num_valid;
      ACE_DEBUG((LM_DEBUG,
                 "Read Publication BIT with key: %x %x %x and handle %d\n"
                 "\tParticipant's key: %x %x %x\n\tTopic: %C\tType: %C\n",
                 data[i].key.value[0], data[i].key.value[1],
                 data[i].key.value[2], infos[i].instance_handle,
                 data[i].participant_key.value[0],
                 data[i].participant_key.value[1],
                 data[i].participant_key.value[2], data[i].topic_name.in(),
                 data[i].type_name.in()));
      if (data[i].user_data.value.length() != 1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          "ERROR publication [%d] user data length %d "
                          "not expected length of 1\n",
                          i,
                          data[i].user_data.value.length()),
                         false);
      }
      if (data[i].topic_data.value.length() != 1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          "ERROR publication [%d] topic data length %d "
                          "not expected length of 1\n",
                          i,
                          data[i].topic_data.value.length()),
                         false);
      }
      if (i != data.length() - 1) {
        continue;
      }
      if (data[i].user_data.value[0] != user_data) {
        ACE_ERROR_RETURN((LM_ERROR,
                          "ERROR publication [%d] user data value %d "
                          "not expected value %d\n",
                          i,
                          data[i].user_data.value[0],
                          user_data),
                         false);
      }
      if (data[i].topic_data.value[0] != topic_data) {
        ACE_ERROR_RETURN((LM_ERROR,
                          "ERROR publication [%d] topic data value %d "
                          "not expected value %d\n",
                          i,
                          data[i].topic_data.value[0],
                          topic_data),
                         false);
      }
      handle = infos[i].instance_handle;
    }
  }
  if (num_valid != num_expected) {
    ACE_ERROR_RETURN((LM_ERROR, "ERROR expected %d discovered "
                                "publications, found %d\n", 
                                num_expected, num_valid), false);
  }
  return true;
}

bool read_subscription_bit(const Subscriber_var& bit_sub,
                           InstanceHandle_t& handle,
                           int user_data,
                           int topic_data,
                           int num_expected = 1)
{
  DataReader_var dr = bit_sub->lookup_datareader(BUILT_IN_SUBSCRIPTION_TOPIC);
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
    ACE_DEBUG((LM_DEBUG,
      "ERROR: (subscription BIT) could not wait for condition: %d\n", result));
    return false;
  }

  SubscriptionBuiltinTopicDataDataReader_var pub_bit =
    SubscriptionBuiltinTopicDataDataReader::_narrow(dr);

  SubscriptionBuiltinTopicDataSeq data;
  SampleInfoSeq infos;
  ReturnCode_t ret =
    pub_bit->read_w_condition(data, infos, LENGTH_UNLIMITED, rc);
  if (ret != RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG, "ERROR: could not read subscription BIT: %d\n", ret));
    return false;
  }

  int num_valid = 0;
  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (infos[i].valid_data) {
      ++num_valid;
      ACE_DEBUG((LM_DEBUG,
                 "Read Subscription BIT with key: %x %x %x and handle %d\n"
                 "\tParticipant's key: %x %x %x\n\tTopic: %C\tType: %C\n",
                 data[i].key.value[0], data[i].key.value[1],
                 data[i].key.value[2], infos[i].instance_handle,
                 data[i].participant_key.value[0],
                 data[i].participant_key.value[1],
                 data[i].participant_key.value[2], data[i].topic_name.in(),
                 data[i].type_name.in()));
      if (data[i].user_data.value.length() != 1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          "ERROR subscription [%d] user data length %d "
                          "not expected length of 1\n",
                          i,
                          data[i].user_data.value.length()),
                         false);
      }
      if (data[i].topic_data.value.length() != 1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          "ERROR subscription [%d] topic data length %d "
                          "not expected length of 1\n",
                          i,
                          data[i].topic_data.value.length()),
                         false);
      }
      if (i != data.length() - 1) {
        continue;
      }
      if (data[i].user_data.value[0] != user_data) {
        ACE_ERROR_RETURN((LM_ERROR,
                          "ERROR subscription [%d] user data value %d "
                          "not expected value %d\n",
                          i,
                          data[i].user_data.value[0],
                          user_data),
                         false);
      }
      if (data[i].topic_data.value[0] != topic_data) {
        ACE_ERROR_RETURN((LM_ERROR,
                          "ERROR subscription [%d] topic data value %d "
                          "not expected value %d\n",
                          i,
                          data[i].topic_data.value[0],
                          topic_data),
                         false);
      }
      handle = infos[i].instance_handle;
    }
  }
  if (num_valid != num_expected) {
    ACE_ERROR_RETURN((LM_ERROR, "ERROR expected %d discovered "
                                "subscriptionsd, found %d\n", 
                                num_expected, num_valid), false);
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
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR expected to discover")
                                  ACE_TEXT("one other participant handle but ")
                                  ACE_TEXT("found %d\n"), len), false);
    } else if (part_handles[0] == my_handle) {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR discovered own ")
                                  ACE_TEXT("participant handle\n")), false);
    } else {
      DDS::ParticipantBuiltinTopicData data;
      dp->get_discovered_participant_data(data, part_handles[0]);
      OpenDDS::DCPS::Discovery_rch disc =
          TheServiceParticipant->get_discovery(dp->get_domain_id());
      OpenDDS::DCPS::DomainParticipantImpl* dp_impl =
          dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(dp.in());

      OpenDDS::DCPS::RepoId repo_id = disc->bit_key_to_repo_id(
          dp_impl,
          OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC,
          data.key);
      if (dp_impl->get_handle(repo_id) != part_handles[0]) {
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR discovered participant ")
                                    ACE_TEXT("BIT key could not be converted ")
                                    ACE_TEXT("to repo id, then handle\n")),
                         false);
      }
      handle = part_handles[0];
    }
  }
  return (stat == RETCODE_OK);
}

bool run_test(DomainParticipant_var& dp,
              DomainParticipant_var& dp2)
{

  // If we are running with an rtps_udp transport, it can't be shared between
  // participants.
  TransportConfig_rch cfg = TheTransportRegistry->get_config("dp1");
  if (!cfg.is_nil()) {
    TheTransportRegistry->bind_config(cfg, dp);
  }
  cfg = TheTransportRegistry->get_config("dp2");
  if (!cfg.is_nil()) {
    TheTransportRegistry->bind_config(cfg, dp2);
  }

  Subscriber_var bit_sub = dp->get_builtin_subscriber();

  if (!read_participant_bit(bit_sub, 128)) {
    return false;
  }

  // Each domain participant's handle to the other
  InstanceHandle_t dp_ih, dp2_ih;
  InstanceHandle_t pub_ih, sub_ih, ig_ih;

  if (!(check_discovered_participants(dp, dp2_ih) &&
        check_discovered_participants(dp2, dp_ih)))
  {
    return false;
  }

  DataWriter_var dw = create_data_writer(dp2);
  if (!dw) {
    ACE_DEBUG((LM_DEBUG, "ERROR: could not create Data Writer (participant 2)\n"));
    return false;
  }

  if (!read_publication_bit(bit_sub, pub_ih, 2, 1)) {
    return false;
  }

  DataReader_var dr = create_data_reader(dp);
  if (!dr) {
    ACE_DEBUG((LM_DEBUG, "ERROR: could not create Data Reader (participant 1)\n"));
    return false;
  }
  if (!read_subscription_bit(dp2->get_builtin_subscriber(), sub_ih, 4, 1)) {
    return false;
  }

  WriterSync::wait_match(dw);

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
    ACE_DEBUG((LM_DEBUG,
      "ERROR: TestMsg reader could not wait for condition: %d\n", result));
    return false;
  }

  TestMsgDataReader_var tmdr = TestMsgDataReader::_narrow(dr);

  TestMsgSeq data;
  SampleInfoSeq infos;
  ReturnCode_t ret = tmdr->read_w_condition(data, infos, LENGTH_UNLIMITED, rc);
  if (ret != RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG, "ERROR: could not read TestMsg: %d\n", ret));
    return false;
  }

  bool ok = false;
  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (infos[i].valid_data) {
      ok = true;
      ACE_DEBUG((LM_DEBUG, "Read data sample: %d\n", data[i].value));
    }
  }

  if (!ok) {
    ACE_DEBUG((LM_DEBUG, "ERROR: no valid data from TestMsg data reader\n"));
  }

  // Change dp qos
  {
    DomainParticipantQos dp_qos;
    dp2->get_qos(dp_qos);
    set_qos(dp_qos.user_data.value, 8);
    dp2->set_qos(dp_qos);
  }
  // Change dw qos
  {
    DataWriterQos dw_qos;
    dw->get_qos(dw_qos);
    set_qos(dw_qos.user_data.value, 16);
    dw->set_qos(dw_qos);
  }
  // Change dr qos
  {
    DataReaderQos dr_qos;
    dr->get_qos(dr_qos);
    set_qos(dr_qos.user_data.value, 32);
    dr->set_qos(dr_qos);
  }
  // Wait for propagation
  ACE_OS::sleep(3);
  if (!read_participant_bit(bit_sub, 8)) {
    return false;
  }
  if (!read_publication_bit(bit_sub, ig_ih, 16, 1)) {
    return false;
  }
  if (!read_subscription_bit(dp2->get_builtin_subscriber(), ig_ih, 32, 1)) {
    return false;
  }

  // Set dw topic qos
  Topic_var topic = dw->get_topic();
  TopicQos topic_qos;
  topic->get_qos(topic_qos);
  set_qos(topic_qos.topic_data.value, 64);
  topic->set_qos(topic_qos);

  // Set dr topic qos
  TopicDescription_var topic_desc = dr->get_topicdescription();
  topic = Topic::_narrow(topic_desc);
  topic->get_qos(topic_qos);
  set_qos(topic_qos.topic_data.value, 64);
  topic->set_qos(topic_qos);

  // Wait for propagation
  ACE_OS::sleep(3);
  if (!read_publication_bit(bit_sub, ig_ih, 16, 64)) {
    return false;
  }
  if (!read_subscription_bit(dp2->get_builtin_subscriber(), ig_ih, 32, 64)) {
    return false;
  }

  // Test ignore
  dp->ignore_publication(pub_ih);
  if (!read_publication_bit(bit_sub, pub_ih, 16, 64, 0)) {
    ACE_ERROR_RETURN((LM_ERROR, 
                     ACE_TEXT("Could not ignore publication\n")), false);
  }

  dp2->ignore_subscription(sub_ih);
  if (!read_subscription_bit(dp2->get_builtin_subscriber(), sub_ih, 32, 64, 0)) {
    ACE_ERROR_RETURN((LM_ERROR, 
                     ACE_TEXT("Could not ignore subscription\n")), false);
  }
  
  dp->ignore_participant(dp2_ih);
  InstanceHandleSeq handles;
  dp->get_discovered_participants(handles);
  if (handles.length()) {
    ACE_ERROR_RETURN((LM_ERROR, 
                     ACE_TEXT("Could not ignore participant\n")), false);
  }

  return ok;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  bool ok = false;
  DomainParticipantFactory_var dpf;
  DomainParticipant_var dp, dp2;
  try {
    dpf = TheParticipantFactoryWithArgs(argc, argv);
    dp = dpf->create_participant(9, PARTICIPANT_QOS_DEFAULT,
                                 0, DEFAULT_STATUS_MASK);
    if (!dp) {
      ACE_DEBUG((LM_DEBUG, "ERROR: could not create Domain Participant 1\n"));

    } else {
      DomainParticipantQos dp_qos = PARTICIPANT_QOS_DEFAULT;
      set_qos(dp_qos.user_data.value, 128);
      dp2 = dpf->create_participant(9, dp_qos, 0, DEFAULT_STATUS_MASK);

      if (!dp2) {
        ACE_DEBUG((LM_DEBUG, "ERROR: could not create Domain Participant 2\n"));

      } else {
        ok = run_test(dp, dp2);

        if (!ok) {
          ACE_DEBUG((LM_ERROR, "ERROR from run_test\n"));
          return -1;
        }
      }
    }
  } catch (const std::exception& e) {
    ACE_DEBUG((LM_ERROR, "ERROR: Exception thrown: %C\n", e.what()));
    return -2;
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("ERROR: Exception thrown:");
    return -2;
  } catch (const OpenDDS::DCPS::Transport::Exception&) {
    ACE_DEBUG((LM_ERROR, "ERROR: Transport exception thrown\n"));
    return -2;
  } catch (...) {
    ACE_DEBUG((LM_ERROR, "ERROR: unknown exception thrown\n"));
    return -2;
  }

  ACE_DEBUG((LM_INFO, "Cleaning up test\n"));
  cleanup(dpf, dp);
  ACE_OS::sleep(2);
  cleanup(dpf, dp2);
  TheServiceParticipant->shutdown();
  ACE_Thread_Manager::instance()->wait();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
