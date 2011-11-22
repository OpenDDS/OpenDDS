#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DdsDcpsInfrastructureC.h"

using namespace DDS;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  DomainParticipant_var dp = dpf->create_participant(9, PARTICIPANT_QOS_DEFAULT,
                                                     0, DEFAULT_STATUS_MASK);

  if (!dp) {
    ACE_DEBUG((LM_DEBUG, "ERROR: could not create Domain Participant\n"));
    return 1;
  }

  ACE_OS::sleep(120);

  ReturnCode_t ret = dp->delete_contained_entities();
  if (ret != RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG, "ERROR: delete_contained_entities() returned %d\n",
               ret));
  }

  ret = dpf->delete_participant(dp);
  if (ret != RETCODE_OK) {
    ACE_DEBUG((LM_DEBUG, "ERROR: delete_participant() returned %d\n", ret));
  }

  TheServiceParticipant->shutdown();
  return ret;
}
