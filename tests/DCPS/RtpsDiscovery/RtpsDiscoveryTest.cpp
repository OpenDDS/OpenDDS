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
  ACE_OS::sleep(120);
  dpf->delete_participant(dp);
  TheServiceParticipant->shutdown();
  return 0;
}
