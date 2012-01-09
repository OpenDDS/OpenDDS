#include "TestMsgTypeSupportImpl.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/WaitSet.h"

#include "dds/DdsDcpsInfrastructureC.h"

#include "model/Sync.h"

using namespace DDS;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;
using OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC;
using OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC;
using OpenDDS::Model::WriterSync;

void cleanup(const DDS::DomainParticipantFactory_var& dpf,
             const DDS::DomainParticipant_var& dp)
{
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

bool read_participant_bit(const DDS::Subscriber_var& bit_sub)
{
  DataReader_var dr = bit_sub->lookup_datareader(BUILT_IN_PARTICIPANT_TOPIC);
  ReadCondition_var rc = dr->create_readcondition(ANY_SAMPLE_STATE,
                                                  ANY_VIEW_STATE,
                                                  ALIVE_INSTANCE_STATE);
  WaitSet_var waiter = new WaitSet;
  waiter->attach_condition(rc);
  ConditionSeq activeConditions;
  Duration_t forever = { DDS::DURATION_INFINITE_SEC,
                         DDS::DURATION_INFINITE_NSEC };
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
  ACE_OS::sleep(20);
  ReturnCode_t ret = part_bit->read(data, infos, LENGTH_UNLIMITED,
                                    ANY_SAMPLE_STATE, ANY_VIEW_STATE,
                                    ALIVE_INSTANCE_STATE);
  if (ret != RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG, "ERROR: could not read participant BIT: %d\n", ret));
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

DDS::DataWriter_var create_data_writer(const DDS::DomainParticipant_var& dp2)
{
  DDS::TypeSupport_var ts = new TestMsgTypeSupportImpl;

  if (ts->register_type(dp2, "") != DDS::RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to register type support"));
    return 0;
  }

  CORBA::String_var type_name = ts->get_type_name();
  DDS::Topic_var topic = dp2->create_topic("Movie Discussion List",
                                           type_name,
                                           TOPIC_QOS_DEFAULT,
                                           0,
                                           DEFAULT_STATUS_MASK);

  if (!topic) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to create topic"));
    return 0;
  }

  DDS::Publisher_var pub = dp2->create_publisher(PUBLISHER_QOS_DEFAULT,
                                                 0,
                                                 DEFAULT_STATUS_MASK);

  if (!pub) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to create publisher"));
    return 0;
  }

  DDS::DataWriter_var dw = pub->create_datawriter(topic,
                                                  DATAWRITER_QOS_DEFAULT,
                                                  0,
                                                  DEFAULT_STATUS_MASK);

  if (!dw) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to create data writer"));
    return false;
  }
  return dw;
}

DDS::DataReader_var create_data_reader(const DDS::DomainParticipant_var& dp)
{
  DDS::TypeSupport_var ts = new TestMsgTypeSupportImpl;

  if (ts->register_type(dp, "") != DDS::RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to register type support"));
    return 0;
  }

  CORBA::String_var type_name = ts->get_type_name();
  DDS::Topic_var topic = dp->create_topic("Movie Discussion List",
                                          type_name,
                                          TOPIC_QOS_DEFAULT,
                                          0,
                                          DEFAULT_STATUS_MASK);

  if (!topic) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to create topic"));
    return 0;
  }

  DDS::Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                                  0,
                                                  DEFAULT_STATUS_MASK);

  if (!sub) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to create subscriber"));
    return 0;
  }

  DDS::DataReader_var dr = sub->create_datareader(topic,
                                                  DATAREADER_QOS_DEFAULT,
                                                  0,
                                                  DEFAULT_STATUS_MASK);

  if (!dr) {
    ACE_DEBUG((LM_DEBUG, "ERROR: failed to create data reader"));
    return 0;
  }
  return dr;
}

bool read_publication_bit(const DDS::Subscriber_var& bit_sub)
{
  DataReader_var dr = bit_sub->lookup_datareader(BUILT_IN_PUBLICATION_TOPIC);
  ReadCondition_var rc = dr->create_readcondition(ANY_SAMPLE_STATE,
                                                  ANY_VIEW_STATE,
                                                  ALIVE_INSTANCE_STATE);
  WaitSet_var waiter = new WaitSet;
  waiter->attach_condition(rc);
  ConditionSeq activeConditions;
  const Duration_t ten_seconds = { 10, 0 };
  ReturnCode_t result = waiter->wait(activeConditions, ten_seconds);
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
  ReturnCode_t ret = pub_bit->read(data, infos, LENGTH_UNLIMITED,
                                   ANY_SAMPLE_STATE, ANY_VIEW_STATE,
                                   ALIVE_INSTANCE_STATE);
  if (ret != RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG, "ERROR: could not read publication BIT: %d\n", ret));
  }

  for (CORBA::ULong i = 0; i < data.length(); ++i) {
    if (infos[i].valid_data) {
      ACE_DEBUG((LM_DEBUG,
                 "Read Publication BIT with key: %x %x %x and handle %d\n"
                 "\tParticipant's key: %x %x %x\n"
                 "\tTopic: %C\tType: %C\n",
                 data[i].key.value[0],
                 data[i].key.value[1],
                 data[i].key.value[2],
                 infos[i].instance_handle,
                 data[i].participant_key.value[0],
                 data[i].participant_key.value[1],
                 data[i].participant_key.value[2],
                 data[i].topic_name.in(),
                 data[i].type_name.in()));
    }
  }
  return true;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DomainParticipant_var dp = dpf->create_participant(9, PARTICIPANT_QOS_DEFAULT,
                                                     0, DEFAULT_STATUS_MASK);

  if (!dp) {
    ACE_DEBUG((LM_DEBUG, "ERROR: could not create Domain Participant 1\n"));
    return 1;
  }

  DomainParticipant_var dp2 =
    dpf->create_participant(9, PARTICIPANT_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);

  if (!dp2) {
    ACE_DEBUG((LM_DEBUG, "ERROR: could not create Domain Participant 2\n"));
    return 1;
  }

  Subscriber_var bit_sub = dp->get_builtin_subscriber();

  read_participant_bit(bit_sub);

  DDS::DataWriter_var dw = create_data_writer(dp2);
  if (!dw) {
    ACE_DEBUG((LM_DEBUG, "ERROR: could not create Data Writer (participant 2)\n"));
    return 1;
  }

  read_publication_bit(bit_sub);

  DDS::DataReader_var dr = create_data_reader(dp);
  if (!dr) {
    ACE_DEBUG((LM_DEBUG, "ERROR: could not create Data Reader (participant 1)\n"));
    return 1;
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
  const Duration_t ten_seconds = { 10, 0 };
  ReturnCode_t result = waiter->wait(activeConditions, ten_seconds);
  waiter->detach_condition(rc);
  if (result != RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG,
      "ERROR: TestMsg reader could not wait for condition: %d\n", result));
    return 1;
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

  ACE_DEBUG((LM_INFO, "Cleaning up test\n"));
  ACE_OS::sleep(10);
  cleanup(dpf, dp);
  ACE_OS::sleep(5);
  cleanup(dpf, dp2);
  TheServiceParticipant->shutdown();
  ACE_Thread_Manager::instance()->wait();
  return 0;
}
