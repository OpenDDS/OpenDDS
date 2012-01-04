#include "TestMsgTypeSupportImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/WaitSet.h"

#include "dds/DdsDcpsInfrastructureC.h"

using namespace DDS;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;
using OpenDDS::DCPS::BUILT_IN_PARTICIPANT_TOPIC;

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

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  TestMsgTypeSupport_var ts;
  DDS::Topic_var topic;
  DDS::Publisher_var pub;
  DDS::DataWriter_var dw;

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

  {
    Subscriber_var bit_sub = dp->get_builtin_subscriber();
    DataReader_var dr = bit_sub->lookup_datareader(BUILT_IN_PARTICIPANT_TOPIC);
    ReadCondition_var rc = dr->create_readcondition(ANY_SAMPLE_STATE,
                                                    ANY_VIEW_STATE,
                                                    ALIVE_INSTANCE_STATE);
    WaitSet waiter;
    waiter.attach_condition(rc);
    ConditionSeq activeConditions;
    Duration_t forever = { DDS::DURATION_INFINITE_SEC,
                           DDS::DURATION_INFINITE_NSEC };
    ReturnCode_t result = waiter.wait(activeConditions, forever);
    if (result != RETCODE_OK) {
      ACE_DEBUG((LM_DEBUG, "ERROR: could not wait for condition: %d\n", result));
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

    if (data.length() > 0) {
      for (CORBA::ULong i = 0; i < data.length(); ++i) {
        if (infos[i].valid_data) {
          if (&data[i] == 0)
            ACE_DEBUG((LM_DEBUG, "ERROR: key value is empty!\n"));
          else
            ACE_DEBUG((LM_DEBUG, "Read Participant BIT with key: %x %x %x and handle %d\n",
                       data[i].key.value[0],
                       data[i].key.value[1],
                       data[i].key.value[2],
                       infos[i].instance_handle));
        }
      }
    }

    part_bit->return_loan(data, infos);
  }

  {
    // Register TypeSupport (Messenger::Message)
    ts = new TestMsgTypeSupportImpl;

    if (ts->register_type(dp2, "") != DDS::RETCODE_OK) {
      throw std::string("failed to register type support");
    }

    // Create Topic (Movie Discussion List)
    CORBA::String_var type_name = ts->get_type_name();
    topic = dp2->create_topic("Movie Discussion List",
                              type_name,
                              TOPIC_QOS_DEFAULT,
                              0,
                              DEFAULT_STATUS_MASK);

    // Check for failure
    if (!topic) {
      throw std::string("failed to create topic");
    }

    // Create Publisher
    pub = dp2->create_publisher(PUBLISHER_QOS_DEFAULT,
                                0,
                                DEFAULT_STATUS_MASK);

    // Check for failure
    if (!pub) {
      throw std::string("failed to create publisher");
    }

    // Create DataWriter
    dw = pub->create_datawriter(topic,
                                DATAWRITER_QOS_DEFAULT,
                                0,
                                DEFAULT_STATUS_MASK);

    // Check for failure
    if (!dw) {
      throw std::string("failed to create data writer");
    }
  }

  ACE_DEBUG((LM_INFO, "Cleaning up test\n"));

  ACE_OS::sleep(10);
  cleanup(dpf, dp);
  ACE_OS::sleep(5);
  cleanup(dpf, dp2);
  TheServiceParticipant->shutdown();
  return 0;
}
