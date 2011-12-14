#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/BuiltInTopicUtils.h"

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

  ACE_OS::sleep(60);

  {
    Subscriber_var bit_sub = dp->get_builtin_subscriber();
    DataReader_var dr = bit_sub->lookup_datareader(BUILT_IN_PARTICIPANT_TOPIC);
    ParticipantBuiltinTopicDataDataReader_var part_bit =
      ParticipantBuiltinTopicDataDataReader::_narrow(dr);
    ParticipantBuiltinTopicDataSeq data;
    SampleInfoSeq infos;
    ReturnCode_t ret = part_bit->read(data, infos, 1, ANY_SAMPLE_STATE,
                                      ANY_VIEW_STATE, ALIVE_INSTANCE_STATE);
    if (ret != RETCODE_OK) {
      ACE_DEBUG((LM_DEBUG, "ERROR: could not read participant BIT: %d\n", ret));
    }

    if (data.length() > 0) {
      if (&data[0] == 0)
        ACE_DEBUG((LM_DEBUG, "ERROR: key value is empty!\n"));
      else
        ACE_DEBUG((LM_DEBUG, "Read Participant BIT with key: %x %x %x\n",
                   data[0].key.value[0],
                   data[0].key.value[1],
                   data[0].key.value[2]));
    }

    part_bit->return_loan(data, infos);
  }

  ACE_OS::sleep(60);

  cleanup(dpf, dp);
  cleanup(dpf, dp2);

  TheServiceParticipant->shutdown();
  return 0;
}
