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

bool read_participant_bit(const Subscriber_var& bit_sub)
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

  CORBA::String_var type_name = ts->get_type_name();
  Topic_var topic = dp2->create_topic("Movie Discussion List",
                                      type_name,
                                      TOPIC_QOS_DEFAULT,
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

  DataWriter_var dw = pub->create_datawriter(topic,
                                             DATAWRITER_QOS_DEFAULT,
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

  CORBA::String_var type_name = ts->get_type_name();
  Topic_var topic = dp->create_topic("Movie Discussion List",
                                     type_name,
                                     TOPIC_QOS_DEFAULT,
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

  DataReader_var dr = sub->create_datareader(topic,
                                             DATAREADER_QOS_DEFAULT,
                                             0,
                                             DEFAULT_STATUS_MASK);

  if (!dr) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to create data reader"));
    return 0;
  }
  return dr;
}

bool read_publication_bit(const Subscriber_var& bit_sub)
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

  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (infos[i].valid_data) {
      ACE_DEBUG((LM_DEBUG,
                 "Read Publication BIT with key: %x %x %x and handle %d\n"
                 "\tParticipant's key: %x %x %x\n\tTopic: %C\tType: %C\n",
                 data[i].key.value[0], data[i].key.value[1],
                 data[i].key.value[2], infos[i].instance_handle,
                 data[i].participant_key.value[0],
                 data[i].participant_key.value[1],
                 data[i].participant_key.value[2], data[i].topic_name.in(),
                 data[i].type_name.in()));
    }
  }
  return true;
}

bool run_test(const DomainParticipant_var& dp,
              const DomainParticipant_var& dp2)
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

  read_participant_bit(bit_sub);

  DataWriter_var dw = create_data_writer(dp2);
  if (!dw) {
    ACE_DEBUG((LM_DEBUG, "ERROR: could not create Data Writer (participant 2)\n"));
    return false;
  }

  read_publication_bit(bit_sub);

  DataReader_var dr = create_data_reader(dp);
  if (!dr) {
    ACE_DEBUG((LM_DEBUG, "ERROR: could not create Data Reader (participant 1)\n"));
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

  Topic_var topic = dw->get_topic();
  TopicQos topic_qos;
  topic->get_qos(topic_qos);
  topic_qos.topic_data.value.length(1);
  topic_qos.topic_data.value[0] = 7;
  topic->set_qos(topic_qos);

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
      dp2 = dpf->create_participant(9, PARTICIPANT_QOS_DEFAULT,
                                    0, DEFAULT_STATUS_MASK);

      if (!dp2) {
        ACE_DEBUG((LM_DEBUG, "ERROR: could not create Domain Participant 2\n"));

      } else {
        ok = run_test(dp, dp2);

        if (!ok) {
          ACE_DEBUG((LM_ERROR, "ERROR from run_test\n"));
        }
      }
    }
  } catch (const std::exception& e) {
    ACE_DEBUG((LM_ERROR, "ERROR: Exception thrown: %C\n", e.what()));
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("ERROR: Exception thrown:");
  } catch (const OpenDDS::DCPS::Transport::Exception&) {
    ACE_DEBUG((LM_ERROR, "ERROR: Transport exception thrown\n"));
  } catch (...) {
    ACE_DEBUG((LM_ERROR, "ERROR: unknown exception thrown\n"));
  }

  ACE_DEBUG((LM_INFO, "Cleaning up test\n"));
  cleanup(dpf, dp);
  ACE_OS::sleep(2);
  cleanup(dpf, dp2);
  TheServiceParticipant->shutdown();
  ACE_Thread_Manager::instance()->wait();
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
